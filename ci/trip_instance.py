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
