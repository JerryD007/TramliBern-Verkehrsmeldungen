import unittest
from gtfs_alert_segments import Index, read_gtfs
from trip_instance import resolve_original_instance
import test_static_integrity


class OriginalTripResolution(unittest.TestCase):
    def setUp(self):
        self.original = 'ch:1:sjyid:100001:1-001'
        self.g = Index(routes={'r': {'agency_id':'11'}}, trips={
            'a': {'original_trip_id':self.original,'service_id':'a','route_id':'r'},
            'b': {'original_trip_id':self.original,'service_id':'b','route_id':'r'}},
            original_trips={self.original:{'a','b'}},
            exceptions={('a','20261023'):'1', ('b','20261023'):'2'},
            first_departures={'a':(1,'10:00:00'),'b':(1,'10:00:00')})
        self.s = dict(tripId=self.original,originalTripId=self.original,
                      startDate='20261023',startTime='10:00:00',agencyId='11')
    def test_calendar_disambiguates_explicit_original_id(self):
        self.assertEqual(resolve_original_instance(self.g,self.s)[0], 'a')
    def test_two_matches_are_not_guessed(self):
        self.g.exceptions[('b','20261023')]='1'
        self.assertEqual(resolve_original_instance(self.g,self.s), (None,'SJYID ambiguous'))
    def test_unknown_alternative_blocks_unique_known_match(self):
        del self.g.exceptions[('b','20261023')]
        self.assertIsNone(resolve_original_instance(self.g,self.s)[0])
    def test_missing_date_and_time_are_not_inferred(self):
        for field in ('startDate','startTime'):
            s=dict(self.s);del s[field]
            self.assertIsNone(resolve_original_instance(self.g,s)[0])
    def test_conflicting_exact_trip_is_not_repaired(self):
        self.assertIsNone(resolve_original_instance(self.g,dict(self.s,tripId='a'))[0])
    def test_unknown_opaque_trip_is_not_repaired(self):
        self.assertIsNone(resolve_original_instance(self.g,dict(self.s,tripId='.ojp-missing'))[0])
    def test_time_not_wrapped(self):
        self.g.first_departures['a']=(1,'34:00:00')
        self.assertIsNone(resolve_original_instance(self.g,self.s)[0])
    def test_unknown_constraint_blocks_mapping(self):
        self.assertIsNone(resolve_original_instance(self.g,dict(self.s,unknown='x'))[0])
    def test_alias_departure_is_loaded_outside_selected_station(self):
        data=test_static_integrity.StaticIntegrity().data(
            trips='route_id,trip_id,original_trip_id\nr,t,'+self.original+'\n',
            stops='stop_id,location_type,parent_station\nstation,1,\np,0,station\nother,0,\n',
            stop_times='trip_id,stop_id,stop_sequence,departure_time\nt,other,1,10:00:00\nt,p,2,10:10:00\n')
        g=read_gtfs(data,['station'],{self.original})
        self.assertEqual(g.original_trips[self.original], {'t'})
        self.assertEqual(g.first_departures['t'], (1,'10:00:00'))
