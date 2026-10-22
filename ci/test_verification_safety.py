import unittest
from gtfs_alert_segments import Index, evaluate_selector
from alert_decode import decode_alerts, active_periods

class Safety(unittest.TestCase):
    def setUp(self):
        self.g=Index(routes={'r':{'agency_id':'a','route_type':'3'}},
            stops={'station':{'location_type':'1'},'p':{'parent_station':'station'},
                   'station:fake':{},'q':{'parent_station':'station'}},
            trips={'t':{'route_id':'r','direction_id':'0'}},visits={'t':{1:'p'}})
    def check(self, **selector): return evaluate_selector(self.g,selector,'station')[0]
    def test_declared_parent(self): self.assertEqual(self.check(stopId='p'), 'confirmed_stop')
    def test_prefix_not_parent(self): self.assertEqual(self.check(stopId='station:fake'), 'no_match')
    def test_station_selector_child_visit(self): self.assertEqual(self.check(stopId='station',tripId='t'),'confirmed_stop')
    def test_wrong_direction(self): self.assertEqual(self.check(stopId='p',routeId='r',directionId=1),'no_match')
    def test_right_direction(self): self.assertEqual(self.check(stopId='p',routeId='r',directionId=0),'confirmed_stop')
    def test_missing_direction(self):
        del self.g.trips['t']['direction_id']
        self.assertEqual(self.check(stopId='p',tripId='t',directionId=0),'unverifiable')
    def test_wrong_agency(self): self.assertEqual(self.check(stopId='p',agencyId='wrong'),'no_match')
    def test_unknown_constraint(self): self.assertEqual(self.check(stopId='p',other=1),'unverifiable')
    def test_no_segment_inference(self): self.assertEqual(self.check(routeId='r',directionId=0),'candidate')
    def test_no_sibling_platform(self):
        self.assertEqual(evaluate_selector(self.g,{'stopId':'p'},'q')[0],'no_match')
    def test_period_end_exclusive(self): self.assertFalse(active_periods([{'start':1,'end':10}],10))
    def test_period_gap(self): self.assertFalse(active_periods([{'end':5},{'start':15}],10))
    def test_inverted_period(self):
        with self.assertRaises(ValueError): active_periods([{'start':20,'end':10}],15)
    def test_invalid_period(self):
        with self.assertRaises(ValueError): active_periods([{'start':True}],15)
    def test_stale_feed(self):
        with self.assertRaises(ValueError): decode_alerts({'header':{'timestamp':1},'entity':[]},1000)
    def test_nested_conflict_preserved(self):
        payload={'header':{'timestamp':1000},'entity':[{'id':'x','alert':{'informedEntity':[
            {'routeId':'r','stopId':'p','trip':{'routeId':'other','tripId':'t'}}]}}]}
        sel=decode_alerts(payload,1000)[0]['selectors'][0]
        self.assertEqual(evaluate_selector(self.g,sel,'station')[0],'unverifiable')
    def test_deleted(self):
        self.assertEqual(decode_alerts({'header':{'timestamp':1000},'entity':[{'id':'x','isDeleted':True}]},1000),[])
    def test_differential(self):
        with self.assertRaises(ValueError): decode_alerts({'header':{'timestamp':1000,'incrementality':1},'entity':[]},1000)
