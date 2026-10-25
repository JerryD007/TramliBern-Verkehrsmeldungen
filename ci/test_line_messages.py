import json
import unittest
from test_gtfs_alert_segments import demo_gtfs, S1, S2, S3
from line_messages import line_match
from verified_feed import build_line_feed
from check_feed_publication import check_feed

class LineMessages(unittest.TestCase):
    def test_same_route_different_stop_is_line_relevance(self):
        self.assertEqual(line_match(demo_gtfs(), {'routeId':'r-7','stopId':S3}, S1)['line'],'7')
    def test_route_only(self):
        self.assertIsNotNone(line_match(demo_gtfs(), {'routeId':'r-7'}, S1))
    def test_unrelated_route(self):
        self.assertIsNone(line_match(demo_gtfs(), {'routeId':'r-28'}, S1))
    def test_no_line_number_guess(self):
        self.assertIsNone(line_match(demo_gtfs(), {'routeId':'7'}, S1))
    def test_stop_only_not_line_message(self):
        self.assertIsNone(line_match(demo_gtfs(), {'stopId':S1}, S1))
    def test_contradictions_and_unknowns_blocked(self):
        for sel in ({'routeId':'r-7','stopId':S2},
                    {'routeId':'r-7','tripId':'t28'},
                    {'routeId':'r-7','unsupportedProtobufFields':True},
                    {'routeId':'r-7','directionId':9}):
            self.assertIsNone(line_match(demo_gtfs(),sel,S1))
    def test_contract_and_scope(self):
        report={'matches':[], 'line_matches':[dict(alert_id='a',stop='O',route_id='r-7',line='7')],
                'selected_stops':{'O':S1},'static':{'sha256':'a'*64}}
        alerts=[dict(id='a',activePeriod=[dict(start=900,end=1100)],
                     headerText={'translation':[{'language':'de','text':'Umleitung'}]})]
        feed=build_line_feed(report,alerts,1000,1000)
        self.assertEqual(feed['schema'],3)
        self.assertEqual(feed['alerts'][0]['text'],'Linienmeldung 7: Umleitung')
        check_feed(json.dumps(feed).encode(),1000)
        feed['alerts'][0]['text']='Haltestelle betroffen'
        with self.assertRaises(ValueError):check_feed(json.dumps(feed).encode(),1000)
        self.assertEqual(build_line_feed(report,alerts,1100,1000)['alerts'],[])
