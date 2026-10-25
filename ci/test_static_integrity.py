import io
import zipfile
import unittest
from gtfs_alert_segments import read_gtfs, evaluate_selector

class StaticIntegrity(unittest.TestCase):
    def data(self, **replacements):
        tables={'routes':'route_id,agency_id\nr,a\n',
                'trips':'route_id,trip_id,direction_id\nr,t,0\n',
                'stops':'stop_id,location_type,parent_station\nstation,1,\np,0,station\n',
                'stop_times':'trip_id,stop_id,stop_sequence\nt,p,1\n'}
        tables.update(replacements)
        buffer=io.BytesIO()
        with zipfile.ZipFile(buffer,'w') as z:
            for name,text in tables.items(): z.writestr(name+'.txt',text)
        return buffer.getvalue()
    def test_parent_join_from_csv(self):
        g=read_gtfs(self.data(),['station'])
        self.assertEqual(evaluate_selector(g,dict(routeId='r',stopId='station',directionId=0),'station')[0],'confirmed_stop')
    def test_duplicate_trip(self):
        with self.assertRaises(ValueError): read_gtfs(self.data(trips='route_id,trip_id\nr,t\nr,t\n'))
    def test_duplicate_stop(self):
        with self.assertRaises(ValueError): read_gtfs(self.data(stops='stop_id\np\np\n'))
    def test_missing_id(self):
        with self.assertRaises(ValueError): read_gtfs(self.data(routes='route_short_name\n7\n'))
    def test_orphan_visit(self):
        with self.assertRaises(ValueError): read_gtfs(self.data(stop_times='trip_id,stop_id,stop_sequence\nmissing,p,1\n'))
    def test_negative_sequence(self):
        with self.assertRaises(ValueError): read_gtfs(self.data(stop_times='trip_id,stop_id,stop_sequence\nt,p,-1\n'))
    def test_duplicate_sequence(self):
        with self.assertRaises(ValueError): read_gtfs(self.data(stop_times='trip_id,stop_id,stop_sequence\nt,p,1\nt,p,1\n'))
    def test_opaque_ids(self):
        g=read_gtfs(self.data())
        self.assertEqual(evaluate_selector(g,{'stopId':'p'},'station')[0],'confirmed_stop')
    def test_missing_tables(self):
        b=io.BytesIO()
        with zipfile.ZipFile(b,'w'): pass
        with self.assertRaises(ValueError): read_gtfs(b.getvalue())
