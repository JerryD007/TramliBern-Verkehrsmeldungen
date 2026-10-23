import unittest
from verified_feed import build_feed

class VerifiedFeed(unittest.TestCase):
    def setup_data(self,status='explicit_stop'):
        return ({'matches':[{'status':status,'alert_id':'a','stop':'O'}],
                 'selected_stops':{'O':'ch:1:sloid:1'},'static':{'sha256':'abc'}},
                [{'id':'a','activePeriod':[{'start':900,'end':1050}],
                  'headerText':{'translation':[{'language':'de','text':'Haltestelle gesperrt'}]}}])
    def test_confirmed_only(self):
        r,a=self.setup_data(); f=build_feed(r,a,1000,1000)
        self.assertEqual(f['alerts'][0]['end'],1050)
        self.assertEqual(f['schema'],2)
    def test_candidates_never_published(self):
        r,a=self.setup_data('route_candidate'); self.assertEqual(build_feed(r,a,1000,1000)['alerts'],[])
    def test_unknown_never_published(self):
        r,a=self.setup_data('unverifiable'); self.assertEqual(build_feed(r,a,1000,1000)['alerts'],[])
    def test_expired_never_published(self):
        r,a=self.setup_data(); self.assertEqual(build_feed(r,a,1050,1000)['alerts'],[])
    def test_no_text_no_warning(self):
        r,a=self.setup_data(); a[0]['headerText']={}; self.assertEqual(build_feed(r,a,1000,1000)['alerts'],[])
    def test_missing_period_capped(self):
        r,a=self.setup_data(); a[0]['activePeriod']=[]; self.assertEqual(build_feed(r,a,1000,1000)['alerts'][0]['end'],1900)
    def test_duplicate_selectors_deduplicated(self):
        r,a=self.setup_data();r['matches']*=3;self.assertEqual(len(build_feed(r,a,1000,1000)['alerts']),1)
    def test_source_age_not_reset_by_processing(self):
        r,a=self.setup_data();a[0]['activePeriod']=[]
        self.assertEqual(build_feed(r,a,1000,source_timestamp=200)['expires'],1100)
    def test_stale_source_cannot_be_republished(self):
        r,a=self.setup_data()
        with self.assertRaises(ValueError):build_feed(r,a,1000,source_timestamp=100)

    def test_row_limit_fails_instead_of_truncating(self):
        r,a=self.setup_data()
        r['matches']=[dict(status='explicit_stop',alert_id='a',stop=str(i)) for i in range(33)]
        r['selected_stops']={str(i):'ch:1:sloid:'+str(i) for i in range(33)}
        with self.assertRaises(ValueError):build_feed(r,a,1000,1000)
    def test_utf8_byte_limit(self):
        r,a=self.setup_data();a[0]['headerText']['translation'][0]['text']='🚋'*180
        r['matches']=[dict(status='explicit_stop',alert_id='a',stop=str(i)) for i in range(32)]
        r['selected_stops']={str(i):'ch:1:sloid:'+str(i) for i in range(32)}
        with self.assertRaises(ValueError):build_feed(r,a,1000,1000)

    def test_missing_source_timestamp_is_rejected(self):
        r,a=self.setup_data()
        with self.assertRaises(ValueError): build_feed(r,a,1000)

    def test_invalid_source_timestamps_are_rejected(self):
        r,a=self.setup_data()
        for stamp in (None, True, '1000', 1000.0, 1121, -1):
            with self.subTest(stamp=stamp), self.assertRaises(ValueError):
                build_feed(r,a,1000,stamp)
