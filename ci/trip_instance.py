"""Exact scheduled trip-instance checks; unsupported frequency instances fail closed."""
import re
from datetime import datetime

def service_date(value):
    if not isinstance(value,str) or not re.fullmatch(r'\d{8}',value): raise ValueError('invalid service date')
    return datetime.strptime(value,'%Y%m%d').date()

def seconds(value):
    if not isinstance(value,str) or not re.fullmatch(r'\d{2,}:\d{2}:\d{2}',value): raise ValueError('invalid GTFS time')
    h,m,s=map(int,value.split(':'))
    if m>59 or s>59: raise ValueError('invalid GTFS time')
    return h*3600+m*60+s


def resolve_original_instance(g, selector):
    """Diagnostic mapping of legacy Swiss SJYID tripId; no string-ID rewrites.

    Every alias comes from original_trip_id. All candidate calendars/times must
    be available; one successful candidate cannot hide an unresolved alternative.
    Returns a static ID only for a single fully matching instance.
    """
    tid = selector.get('tripId')
    if not isinstance(tid, str):
        return None, 'invalid tripId'
    if set(selector) - {'tripId','originalTripId','startDate','startTime',
                        'scheduleRelationship','routeId','directionId','agencyId','stopId','routeType'}:
        return None, 'unsupported selector constraint'
    if tid in g.trips:
        return None, 'direct tripId must be checked without remapping'
    if (not isinstance(tid, str) or not tid.startswith('ch:1:sjyid:') or
            selector.get('originalTripId') != tid):
        return None, 'not an explicit legacy SJYID selector'
    if not selector.get('startDate') or not selector.get('startTime'):
        return None, 'SJYID requires explicit date and initial departure'
    candidates = g.original_trips.get(tid, set())
    if not candidates:
        return None, 'original_trip_id absent from static'
    matches = []
    unresolved = False
    for candidate in sorted(candidates):
        trip = g.trips[candidate]
        route = g.routes.get(trip.get('route_id'))
        if 'routeId' in selector and selector['routeId'] != trip.get('route_id'):
            continue
        if 'directionId' in selector:
            if type(selector['directionId']) is not int or selector['directionId'] not in (0, 1):
                return None, 'invalid direction constraint'
            if trip.get('direction_id') not in ('0', '1'):
                unresolved = True
                continue
            if str(selector['directionId']) != trip['direction_id']:
                continue
        if 'agencyId' in selector:
            if route is None or not route.get('agency_id'):
                unresolved = True
                continue
            if route['agency_id'] != selector['agencyId']:
                continue
        if 'routeType' in selector:
            if type(selector['routeType']) is not int or selector['routeType'] < 0:
                return None, 'invalid route type constraint'
            if route is None or not route.get('route_type'):
                unresolved = True
                continue
            if route['route_type'] != str(selector['routeType']):
                continue
        result = verify_instance(g, dict(selector, tripId=candidate))
        if result is None:
            matches.append(candidate)
        elif result[0] == 'unverifiable':
            unresolved = True
    if unresolved:
        return None, 'SJYID has unresolved candidate constraints'
    if len(matches) != 1:
        return None, 'SJYID ambiguous' if matches else 'SJYID has no matching instance'
    return matches[0], 'unique original_trip_id/date/startTime instance'

def verify_instance(g, selector):
    fields={'originalTripId','startDate','startTime','scheduleRelationship'} & selector.keys()
    if selector.get('tripId') in g.frequency_trips: return 'unverifiable','frequency trip instance unsupported'
    if not fields: return None
    tid=selector.get('tripId')
    if not tid or tid not in g.trips: return 'unverifiable','trip instance requires exact tripId'
    t=g.trips[tid]
    if 'originalTripId' in selector and (not t.get('original_trip_id') or selector['originalTripId']!=t['original_trip_id']):
        return 'unverifiable','originalTripId conflicts with static trip'
    if type(selector.get('scheduleRelationship')) is bool or selector.get('scheduleRelationship','SCHEDULED') not in ('SCHEDULED',0):
        return 'unverifiable','non-scheduled trip instance unsupported'
    if tid in g.frequency_trips: return 'unverifiable','frequency trip instance unsupported'
    if 'startDate' in selector:
        try: day=service_date(selector['startDate'])
        except ValueError:return 'unverifiable','invalid startDate'
        sid=t.get('service_id')
        exception=g.exceptions.get((sid,selector['startDate']))
        if exception is not None:
            if exception!='1':return 'no_match','trip removed on service date'
        else:
            c=g.calendars.get(sid)
            if c is None: return 'unverifiable','service calendar unavailable'
            try:
                active=(service_date(c['start_date'])<=day<=service_date(c['end_date']) and
                        c[('monday','tuesday','wednesday','thursday','friday','saturday','sunday')[day.weekday()]]=='1')
            except (KeyError,ValueError):return 'unverifiable','invalid service calendar'
            if not active:return 'no_match','trip not scheduled on service date'
    if 'startTime' in selector:
        first=g.first_departures.get(tid)
        if first is None:return 'unverifiable','initial departure unavailable'
        try:
            if seconds(selector['startTime'])!=seconds(first[1]):return 'no_match','startTime differs from initial departure'
        except ValueError:return 'unverifiable','invalid startTime or static departure'
    return None
