"""Atomic full/planned plus authoritative unplanned snapshots.

The full source is intentionally polled infrequently following the Swiss cookbook.
Every response still requires a recent unplanned snapshot. No source timestamps
are renewed by polling or by an HTTP request.
"""
from copy import deepcopy
import threading
from .core import MAX_AGE,response

FULL_MAX_AGE = 86400


class FeedState:
    def __init__(self):
        self._lock = threading.RLock()
        self._feeds = {}
        self._overrides = {}

    def ingest(self, kind, snapshot, now):
        if kind not in ('full','unplanned'):
            raise ValueError('unknown feed kind')
        stamp = snapshot['source_timestamp']
        if not now-MAX_AGE < stamp <= now+120:
            raise ValueError('new acquisition stale or future')
        if not isinstance(snapshot.get('entities'),dict):
            raise ValueError('entity metadata required')
        # Defend against mutations by callers and replace only a complete snapshot.
        snapshot = deepcopy(snapshot)
        with self._lock:
            previous = self._feeds.get(kind)
            if previous:
                old = previous['source_timestamp']
                if stamp < old or (stamp == old and previous != snapshot):
                    raise ValueError('source timestamp rollback or conflicting generation')
            if kind == 'unplanned' and len(set(self._overrides) | set(snapshot['entities']) | set(snapshot.get('blocked_ids',[]))) > 20000:
                raise ValueError('override retention limit')
            self._feeds[kind] = snapshot
            if kind == 'unplanned':
                for sid, entity in snapshot['entities'].items():
                    # Retain removals/updates to planned records until a later full
                    # snapshot supersedes them. Unplanned absences are authoritative.
                    prior = self._overrides.get(sid)
                    version = entity.get('version',-1)
                    if prior is None or version >= prior[1].get('version',-1):
                        self._overrides[sid] = (stamp,entity)
                for sid in snapshot.get('blocked_ids',[]):
                    self._overrides[sid] = (stamp,{'id':sid,'blocked':True,'rows':[]})
            else:
                retained = {}
                for sid,value in self._overrides.items():
                    entry = snapshot['entities'].get(sid)
                    # A newer download must not resurrect an older entity version.
                    if value[0] > stamp or sid in snapshot.get('blocked_ids',[]):
                        retained[sid] = value
                    elif entry is not None:
                        old = value[1]
                        if (entry.get('blocked') or old.get('blocked') or
                            entry.get('version',-1) <= old.get('version',-1)):
                            retained[sid] = value
                self._overrides = retained

    def line_response(self, now, operator, line):
        with self._lock:
            full, realtime = self._feeds.get('full'), self._feeds.get('unplanned')
            if not full or not realtime:
                raise ValueError('both sources required')
            fs, rs = full['source_timestamp'], realtime['source_timestamp']
            if not now-FULL_MAX_AGE < fs <= now+120 or not now-MAX_AGE < rs <= now+120:
                raise ValueError('source expired')
            chosen = {sid:e for sid,e in full['entities'].items() if e.get('planned') is True or e.get('blocked') or e.get('progress') != 'published'}
            # A full snapshot never supplies old unplanned events after they vanish
            # from a newer complete unplanned snapshot.
            for sid, e in realtime['entities'].items():
                chosen[sid] = self._newest(chosen.get(sid),e)
            for sid,(stamp,e) in self._overrides.items():
                if sid in chosen or e.get('planned') is True:
                    chosen[sid] = self._newest(chosen.get(sid),e)
            blocked=set(full.get('blocked_ids',[])) | set(realtime.get('blocked_ids',[]))
            rows=[r for sid,e in chosen.items() if sid not in blocked and not e.get('blocked')
                  for r in e['rows']]
            snapshot={'source_timestamp':rs,'rows':rows,
                      'oversized_situations':full.get('oversized_situations',0)+realtime.get('oversized_situations',0)}
            result=response(snapshot,now,operator,line)
            deadline=min(fs+FULL_MAX_AGE,rs+MAX_AGE)
            result.update(full_source_timestamp=fs,unplanned_source_timestamp=rs,expires_at=deadline)
            for alert in result['alerts']:
                alert['expires_at']=min(alert['expires_at'],deadline)
            return result

    @staticmethod
    def _newest(a,b):
        if a is None:
            return b
        if a.get('blocked') or b.get('blocked'):
            return {'blocked':True,'rows':[]}
        if a.get('participant') != b.get('participant'):
            return {'blocked':True,'rows':[]}
        av,bv=a['version'],b['version']
        if av == bv and a != b:
            return {'blocked':True,'rows':[]}
        return b if bv >= av else a
