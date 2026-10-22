"""Strict test-feed decoding. Unknown constraints remain unverifiable."""
import time


def integer(v):
    if type(v) is int and v >= 0: return v
    if isinstance(v,str) and v.isascii() and v.isdigit(): return int(v)
    raise ValueError('invalid unsigned timestamp')


def active_periods(periods, now):
    if not isinstance(periods,list): raise ValueError('activePeriod must be array')
    if not periods: return True
    active = False
    for p in periods:
        if not isinstance(p,dict) or set(p)-{'start','end'}: raise ValueError('invalid period')
        start = integer(p['start']) if 'start' in p else 0
        end = integer(p['end']) if 'end' in p else None
        if end is not None and end <= start: raise ValueError('inverted/empty period')
        active |= start <= now and (end is None or now < end)
    return active


def normalize(s):
    if not isinstance(s,dict): return {'invalid': True}
    result = dict(s)
    trip = result.pop('trip', None)
    if trip is not None:
        if not isinstance(trip,dict): return {'invalid':True}
        for k,v in trip.items():
            if k not in ('tripId','routeId','directionId','originalTripId','startTime','startDate','scheduleRelationship') or (k in result and result[k] != v):
                return dict(result, unsupportedTripConstraint=trip)
            result[k] = v
    return result


def decode_alerts(payload, now=None):
    now = int(time.time()) if now is None else now
    if not isinstance(payload,dict) or not isinstance(payload.get('entity'),list):
        raise ValueError('invalid GTFS-SA envelope')
    header = payload.get('header',{})
    timestamp = integer(header.get('timestamp'))
    if not now-900 <= timestamp <= now+120:
        raise ValueError(f'stale or future GTFS-SA header: source={timestamp}, now={now}, age_seconds={now-timestamp}')
    if header.get('incrementality', 'FULL_DATASET') not in ('FULL_DATASET',0):
        raise ValueError('differential feed unsupported')
    result=[]
    seen=set()
    for e in payload['entity']:
        if not isinstance(e,dict) or not isinstance(e.get('id'),str) or e['id'] in seen:
            raise ValueError('invalid/duplicate entity id')
        seen.add(e['id'])
        if e.get('isDeleted'): continue
        if 'alert' not in e: continue
        a=e['alert']
        if not isinstance(a,dict): raise ValueError('invalid alert')
        try:
            if not active_periods(a.get('activePeriod',[]),now): continue
        except ValueError as exc:
            result.append({'id':e['id'],'selectors':[],'validation_error':str(exc),
                           'activePeriod':a.get('activePeriod')})
            continue
        sels=a.get('informedEntity',[])
        if not isinstance(sels,list): raise ValueError('invalid informedEntity')
        result.append({'id':e['id'],'selectors':[normalize(s) for s in sels],
                       'activePeriod':a.get('activePeriod',[]),
                       'headerText':a.get('headerText',{}),'descriptionText':a.get('descriptionText',{})})
    return result
