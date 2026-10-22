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

class DecoderRegression(unittest.TestCase):
    def feed(self, alert, **header):
        return {'header':dict(timestamp=1000,**header),'entity':[{'id':'x','alert':alert}]}
    def test_future_period(self):
        self.assertEqual(decode_alerts(self.feed({'activePeriod':[{'start':1100}]}),1000),[])
    def test_absent_period_is_active(self):
        self.assertEqual(len(decode_alerts(self.feed({}),1000)),1)
    def test_no_merge_between_selectors(self):
        sels=[{'routeId':'r'},{'stopId':'p'}]
        self.assertEqual(decode_alerts(self.feed({'informedEntity':sels}),1000)[0]['selectors'],sels)
    def test_trip_start_date_is_not_discarded(self):
        from alert_decode import normalize
        self.assertEqual(normalize({'trip':{'tripId':'t','startDate':'20261022'}})['startDate'],'20261022')
    def test_unknown_top_level_constraint_is_not_discarded(self):
        from alert_decode import normalize
        self.assertIn('newConstraint',normalize({'stopId':'p','newConstraint':'x'}))
    def test_malformed_selector_stays_unverifiable(self):
        from alert_decode import normalize
        self.assertEqual(normalize(None),{'invalid':True})
    def test_future_header(self):
        with self.assertRaises(ValueError): decode_alerts({'header':{'timestamp':2000},'entity':[]},1000)
    def test_missing_header(self):
        with self.assertRaises(ValueError): decode_alerts({'entity':[]},1000)
    def test_duplicate_entity(self):
        p=self.feed({}); p['entity']*=2
        with self.assertRaises(ValueError): decode_alerts(p,1000)
    def test_malformed_period_does_not_default_active(self):
        result=decode_alerts(self.feed({'activePeriod':'bad'}),1000)
        self.assertIn('validation_error',result[0])
        self.assertEqual(result[0]['selectors'],[])
