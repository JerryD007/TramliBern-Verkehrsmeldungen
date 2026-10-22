import unittest
from gtfs_alert_segments import Index,evaluate_selector

class TripInstance(unittest.TestCase):
    def setUp(self):
        self.g=Index(routes={'r':{}},stops={'s':{}},trips={'t':{'route_id':'r','service_id':'daily','original_trip_id':'original'}},visits={'t':{1:'s'}},first_departures={'t':(1,'25:30:00')},calendars={'daily':dict(start_date='20260101',end_date='20261231',monday='1',tuesday='1',wednesday='1',thursday='1',friday='1',saturday='0',sunday='0')})
    def check(self,**kw):return evaluate_selector(self.g,dict(tripId='t',stopId='s',**kw),'s')[0]
    def test_exact_instance(self):self.assertEqual(self.check(startDate='20261022',startTime='25:30:00',originalTripId='original'),'confirmed_stop')
    def test_wrong_original(self):self.assertEqual(self.check(originalTripId='wrong'),'unverifiable')
    def test_invalid_date(self):self.assertEqual(self.check(startDate='20260230'),'unverifiable')
    def test_weekend(self):self.assertEqual(self.check(startDate='20261024'),'no_match')
    def test_removed_service(self):
        self.g.exceptions[('daily','20261022')]='2';self.assertEqual(self.check(startDate='20261022'),'no_match')
    def test_added_service(self):
        self.g.exceptions[('daily','20261024')]='1';self.assertEqual(self.check(startDate='20261024'),'confirmed_stop')
    def test_start_time_not_wrapped(self):self.assertEqual(self.check(startTime='01:30:00'),'no_match')
    def test_missing_calendar(self):
        self.g.calendars={};self.assertEqual(self.check(startDate='20261022'),'unverifiable')
    def test_bad_time(self):self.assertEqual(self.check(startTime='25:61:00'),'unverifiable')
    def test_frequency_not_guessed(self):
        self.g.frequency_trips={'t'};self.assertEqual(self.check(startTime='25:30:00'),'unverifiable')
    def test_new_trip_not_guessed(self):self.assertEqual(self.check(scheduleRelationship='NEW'),'unverifiable')
