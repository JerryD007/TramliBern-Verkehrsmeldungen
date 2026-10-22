"""Historical official-data positive and negative controls; never publish this fixture."""
import csv,io,json,unittest,zipfile
from pathlib import Path
from alert_decode import decode_alerts
from gtfs_alert_segments import read_gtfs,evaluate_selector
from verified_feed import build_feed

ROOT=Path(__file__).parent/'fixtures/real_20261022'
class RealAlertReplay(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        b=io.BytesIO()
        with zipfile.ZipFile(b,'w') as z:
            for name in ('stops.txt','routes.txt','trips.txt','stop_times.txt'):z.writestr(name,(ROOT/name).read_bytes())
        cls.g=read_gtfs(b.getvalue())
        cls.raw=json.loads((ROOT/'alert.json').read_text())
        cls.now=int(cls.raw['header']['timestamp'])
        cls.alert=decode_alerts(cls.raw,now=cls.now)[0]
        cls.sel=next(s for s in cls.alert['selectors'] if s.get('routeId')=='92-12-j26-1' and s.get('stopId')=='ch:1:sloid:90035')
    def test_real_liebegg_positive(self):
        self.assertEqual(evaluate_selector(self.g,self.sel,'ch:1:sloid:90035')[0],'confirmed_stop')
    def test_same_line_does_not_warn_paul_klee(self):
        self.assertEqual(evaluate_selector(self.g,self.sel,'ch:1:sloid:90127')[0],'no_match')
    def test_all_five_targets_excluded(self):
        for sid in ('90090','87480','90127','87481','7061'):
            self.assertEqual(evaluate_selector(self.g,self.sel,'ch:1:sloid:'+sid)[0],'no_match')
    def test_real_positive_feed_at_historical_time(self):
        r={'matches':[{'status':'explicit_stop','alert_id':self.alert['id'],'stop':'Liebegg'}], 'selected_stops':{'Liebegg':'ch:1:sloid:90035'},'static':{'sha256':'fixture'}}
        f=build_feed(r,[self.alert],self.now,self.now)
        self.assertEqual(f['alerts'][0]['stops'],['ch:1:sloid:90035'])
        self.assertTrue(f['alerts'][0]['text'])
    def test_historical_data_cannot_be_republished(self):
        with self.assertRaises(ValueError):decode_alerts(self.raw,now=self.now+901)
    def test_version_mismatch_evidence(self):
        r=json.loads((ROOT/'replay.json').read_text())['route_version_comparison']
        rows=list(csv.DictReader(io.StringIO((ROOT/'routes_2027.csv').read_text())))
        self.assertEqual({x['route_id'] for x in rows},set(r['found_exactly_in_2027']))
        self.assertEqual(len(rows),289)
        self.assertEqual(len(r['unresolved']),11)
