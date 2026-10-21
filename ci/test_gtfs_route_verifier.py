import io
import unittest
import zipfile
from gtfs_route_verifier import verified_candidates


def fixture(trip_stop="ch:1:sloid:90090"):
    streams = {
        "routes.txt": "route_id,route_short_name\\nroute-long-7,7\\nroute-long-9,9\\n",
        "trips.txt": "route_id,service_id,trip_id\\nroute-long-7,WD,trip-A\\nroute-long-9,WD,trip-B\\n",
        "stop_times.txt": "trip_id,arrival_time,departure_time,stop_id,stop_sequence\\ntrip-A,12:00:00,12:00:00,"+trip_stop+",1\\ntrip-B,12:00:00,12:00:00,ch:1:sloid:99999,1\\n",
        "stops.txt": "stop_id,stop_name\\nch:1:sloid:90090,Test Stop\\nch:1:sloid:99999,Other Stop\\n",
    }
    b = io.BytesIO()
    with zipfile.ZipFile(b, "w") as z:
        for name, value in streams.items():
            z.writestr(name, value.replace("\\n", "\n"))
    return b.getvalue()


class VerifiedGtfsJoin(unittest.TestCase):
    def test_exact_route_trip_stop_join(self):
        self.assertEqual(
            verified_candidates(fixture(), ["route-long-7"], ["ch:1:sloid:90090"]),
            {("route-long-7", "ch:1:sloid:90090")})

    def test_no_match_on_similar_route_number(self):
        self.assertEqual(verified_candidates(fixture(), ["7"], ["ch:1:sloid:90090"]), set())

    def test_no_match_on_unrelated_stop(self):
        self.assertEqual(verified_candidates(fixture(), ["route-long-9"], ["ch:1:sloid:90090"]), set())

    def test_invalid_selected_stop_rejected(self):
        with self.assertRaises(ValueError):
            verified_candidates(fixture(), ["route-long-7"], ["90090"])

    def test_unknown_route_rejected(self):
        self.assertEqual(verified_candidates(fixture(), ["unknown"], ["ch:1:sloid:90090"]), set())

    def test_unknown_trip_no_false_match(self):
        self.assertEqual(verified_candidates(fixture("ch:1:sloid:99999"), ["route-long-7"], ["ch:1:sloid:90090"]), set())


if __name__ == "__main__":
    unittest.main()
