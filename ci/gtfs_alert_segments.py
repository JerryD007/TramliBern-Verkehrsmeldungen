"""Fail-closed exact GTFS joins. No segment is inferred from a line number."""
import csv
import io
import zipfile
from dataclasses import dataclass, field

FILES = ('routes.txt', 'trips.txt', 'stops.txt', 'stop_times.txt')

@dataclass
class Index:
    routes: dict = field(default_factory=dict)
    stops: dict = field(default_factory=dict)
    trips: dict = field(default_factory=dict)
    visits: dict = field(default_factory=dict)
    families: dict = field(default_factory=dict)
    aliases: dict = field(default_factory=dict)
    time_rows: list = field(default_factory=list)

    def resolve_stop(self, stop):
        return stop if stop in self.stops else self.aliases.get(stop)

    def family(self, stop):
        stop = self.resolve_stop(stop)
        if stop in self.families:
            return self.families[stop]
        if stop not in self.stops:
            return set()
        # Only a declared station may expand to its declared child platforms.
        if self.stops[stop].get('location_type') == '1':
            self.families[stop] = {stop} | {s for s, r in self.stops.items()
                            if r.get('parent_station') == stop and r.get('location_type', '0') in ('', '0')}
        else:
            self.families[stop] = {stop}
        return self.families[stop]


def rows(z, name):
    with z.open(name) as f:
        yield from csv.DictReader(io.TextIOWrapper(f, encoding='utf-8-sig', newline=''))


def read_gtfs(source, selected_stops=None):
    """Stream stop_times; on live data retain only selected station families."""
    g = Index()
    with zipfile.ZipFile(io.BytesIO(source) if isinstance(source, bytes) else source) as z:
        if not set(FILES).issubset(z.namelist()):
            raise ValueError('GTFS missing required tables')
        for name, key, target in [('routes.txt','route_id',g.routes),
                                  ('stops.txt','stop_id',g.stops),
                                  ('trips.txt','trip_id',g.trips)]:
            for r in rows(z, name):
                ident = r.get(key)
                if not ident or ident in target:
                    raise ValueError('missing or duplicate ' + key)
                target[ident] = ({k:r.get(k,'') for k in ('route_id','direction_id')} if key == 'trip_id' else r)
        originals = {}
        for sid, r in g.stops.items():
            if r.get('location_type') == '1' and r.get('original_stop_id'):
                originals.setdefault(r['original_stop_id'],set()).add(sid)
        g.aliases = {original:next(iter(ids)) for original,ids in originals.items() if len(ids)==1}
        keep = None if selected_stops is None else set().union(*(g.family(s) for s in selected_stops))
        for r in rows(z, 'stop_times.txt'):
            s, t = r.get('stop_id'), r.get('trip_id')
            if keep is not None and s not in keep:
                continue
            if s not in g.stops or t not in g.trips:
                raise ValueError('orphan stop_time')
            try:
                seq = int(r['stop_sequence'])
                if seq < 0: raise ValueError()
            except (KeyError, ValueError):
                raise ValueError('invalid stop_sequence')
            visits = g.visits.setdefault(t, {})
            if seq in visits:
                raise ValueError('duplicate trip stop_sequence')
            visits[seq] = s
            if selected_stops is not None: g.time_rows.append(r)
    return g


def evaluate_selector(g, selector, selected_stop):
    if not isinstance(selector, dict) or not isinstance(selected_stop, str):
        raise ValueError('selector and stop required')
    unknown = ('unverifiable', 'unknown, unsupported or contradictory selector')
    if set(selector) - {'routeId','tripId','stopId','directionId','agencyId','routeType'}:
        return unknown
    if 'agencyId' in selector and (not isinstance(selector['agencyId'],str) or not selector['agencyId']):
        return unknown
    if 'routeType' in selector and (type(selector['routeType']) is not int or selector['routeType'] < 0):
        return unknown
    family = g.family(selected_stop)
    if not family:
        return 'unverifiable', 'selected stop absent from static GTFS'
    route, trip, stop = (selector.get(k) for k in ('routeId','tripId','stopId'))
    for value, table in ((route,g.routes),(trip,g.trips)):
        if value is not None and (not isinstance(value,str) or value not in table):
            return 'unverifiable', 'exact identifier absent from static GTFS'
    if stop is not None and (not isinstance(stop,str) or g.resolve_stop(stop) is None):
        return 'unverifiable', 'exact stop identifier or declared original_stop_id absent'
    direction = selector.get('directionId')
    if direction is not None and (type(direction) is not int or direction not in (0,1) or not (route or trip)):
        return unknown
    if trip and route and g.trips[trip]['route_id'] != route:
        return unknown
    if stop and not (g.family(stop) & family):
        return 'no_match', 'different explicitly named stop'
    if not (route or trip or stop):
        return 'unverifiable', 'no supported spatial identifier'
    candidates = [trip] if trip else g.visits.keys()
    constrained = any(k in selector for k in ('routeId','tripId','directionId','agencyId','routeType'))
    matched = False
    missing_direction = False
    for tid in candidates:
        t = g.trips[tid]
        rid = t.get('route_id')
        r = g.routes.get(rid)
        if r is None or (route and rid != route): continue
        if 'agencyId' in selector and r.get('agency_id') != selector['agencyId']: continue
        if 'routeType' in selector and str(selector['routeType']) != r.get('route_type'): continue
        if direction is not None:
            if t.get('direction_id') not in ('0','1'):
                missing_direction = True
                continue
            if t['direction_id'] != str(direction): continue
        targets = family & g.family(stop) if stop else family
        if targets & set(g.visits.get(tid,{}).values()):
            matched = True
            break
    if constrained and not matched:
        return ('unverifiable', 'direction absent') if missing_direction else ('no_match','no trip satisfies all selector constraints at stop')
    if stop:
        return 'confirmed_stop', 'explicit stop; all supplied constraints verified'
    return 'candidate', 'route or trip visits stop but affected segment unspecified'
