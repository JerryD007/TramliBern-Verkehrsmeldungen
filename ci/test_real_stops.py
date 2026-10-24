"""Real Swiss static stop rows, not evidence of a live disruption."""
import io,zipfile,unittest
from pathlib import Path
from gtfs_alert_segments import read_gtfs

class RealSwissStops(unittest.TestCase):
    def test_original_id_parent_mapping(self):
        buf=io.BytesIO()
        with zipfile.ZipFile(buf,'w') as z:
            z.writestr('stops.txt',(Path(__file__).parent/'fixtures/swiss_selected_stops.csv').read_bytes())
            z.writestr('routes.txt','route_id\n')
            z.writestr('trips.txt','trip_id,route_id\n')
            z.writestr('stop_times.txt','trip_id,stop_id,stop_sequence\n')
        g=read_gtfs(buf.getvalue())
        for id_,name,count in [('90090','Giacomettistrasse',2),('87480','Ostring',3),('90127','Zentrum Paul Klee',2),('87481','Burgernziel',4),('7061','Brunnadernstrasse',6)]:
            selected='ch:1:sloid:'+id_
            parent=g.resolve_stop(selected)
            self.assertEqual(g.stops[parent]['original_stop_id'],selected)
            self.assertEqual(g.stops[parent]['stop_name'],'Bern, '+name)
            self.assertEqual(len(g.family(selected)),count+1)
