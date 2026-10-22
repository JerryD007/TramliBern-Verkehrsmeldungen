import io
import unittest
import zipfile
from gtfs_alert_segments import read_gtfs, evaluate_selector

S1 = "ch:1:sloid:87480"
S2 = "ch:1:sloid:90090"
S3 = "ch:1:sloid:99999"

def demo_gtfs():
    tables = {
        "routes.txt": "route_id,route_short_name\nr-7,7\nr-28,28\n",
        "trips.txt": "route_id,service_id,trip_id\nr-7,x,t7-out\nr-7,x,t7-back\nr-28,x,t28\n",
        "stops.txt": f"stop_id,stop_name\n{S1},Ostring\n{S2},Giacomettistrasse\n{S3},Other\n",
        "stop_times.txt": f"trip_id,stop_id,stop_sequence\nt7-out,{S1},1\nt7-out,{S3},2\nt7-back,{S3},1\nt7-back,{S1},2\nt28,{S2},1\n",
    }
    stream = io.BytesIO()
    with zipfile.ZipFile(stream, "w") as z:
        for filename, content in tables.items():
            z.writestr(filename, content)
    return read_gtfs(stream.getvalue())

class AlertSelectorVerification(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.gtfs = demo_gtfs()

    def check(self, selector, stop=S1):
        return evaluate_selector(self.gtfs, selector, stop)[0]

    def test_explicit_stop_route_and_trip(self):
        self.assertEqual(self.check({"routeId":"r-7", "tripId":"t7-out", "stopId":S1}),"confirmed_stop")

    def test_explicit_stop_only(self):
        self.assertEqual(self.check({"stopId":S1}), "confirmed_stop")

    def test_unrelated_stop(self):
        self.assertEqual(self.check({"routeId":"r-7","stopId":S3}),"no_match")

    def test_route_only_is_not_confirmed(self):
        self.assertEqual(self.check({"routeId":"r-7"}),"candidate")

    def test_trip_only_is_not_confirmed(self):
        self.assertEqual(self.check({"tripId":"t7-out"}),"candidate")

    def test_mismatched_line_number_not_guessed(self):
        self.assertEqual(self.check({"routeId":"7"}),"no_match")

    def test_route_not_at_stop(self):
        self.assertEqual(self.check({"routeId":"r-28"}),"no_match")

    def test_route_and_stop_not_connected(self):
        self.assertEqual(self.check({"routeId":"r-28","stopId":S1}),"no_match")

    def test_unknown_trip(self):
        self.assertEqual(self.check({"tripId":"missing","stopId":S1}),"no_match")

    def test_trip_route_mismatch(self):
        self.assertEqual(self.check({"routeId":"r-28","tripId":"t7-out","stopId":S1}),"no_match")

    def test_trip_wrong_stop(self):
        self.assertEqual(self.check({"tripId":"t28","stopId":S1}),"no_match")

    def test_route_only_not_segment_proof(self):
        status, reason = evaluate_selector(self.gtfs, {"routeId":"r-7"}, S1)
        self.assertEqual(status, "candidate")
        self.assertIn("segment", reason)

    def test_prefix_collision_not_matched(self):
        self.assertEqual(self.check({"stopId":S1}, S1+"0"),"no_match")

if __name__ == "__main__":
    unittest.main()
