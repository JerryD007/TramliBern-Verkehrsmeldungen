import unittest
from gtfs_whole_line import normalize_display_line, route_wide_matches


class WholeLineAlertTests(unittest.TestCase):
    def setUp(self):
        self.routes = {"gtfs-trip-system-123":"7", "real-route-40":"40",
                       "other-route":"8"}

    def test_tram_line_7(self):
        result = route_wide_matches([{"routeId":"gtfs-trip-system-123"}],
                                     self.routes, ["T 7", "B 40"])
        self.assertEqual(result[0]["line"], "7")
        self.assertFalse(result[0]["stop_impact_confirmed"])

    def test_bus_line_40(self):
        self.assertEqual(len(route_wide_matches([{"routeId":"real-route-40"}],
                     self.routes, ["B 40"])), 1)

    def test_other_line_not_shown(self):
        self.assertEqual(route_wide_matches([{"routeId":"other-route"}],
                         self.routes, ["B 40"]), [])

    def test_route_id_is_not_guessed_from_line_number(self):
        self.assertEqual(route_wide_matches([{"routeId":"7"}],
                         self.routes, ["T 7"]), [])

    def test_stop_specific_not_promoted_to_whole_line(self):
        self.assertEqual(route_wide_matches([{"routeId":"real-route-40",
                    "stopId":"ch:1:sloid:90090"}], self.routes, ["B 40"]), [])

    def test_trip_specific_not_promoted(self):
        self.assertEqual(route_wide_matches([{"routeId":"real-route-40",
                    "tripId":"trip-A"}], self.routes, ["B 40"]), [])

    def test_direction_specific_not_promoted(self):
        self.assertEqual(route_wide_matches([{"routeId":"real-route-40",
                    "directionId":1}], self.routes, ["B 40"]), [])

    def test_deduplicate(self):
        self.assertEqual(len(route_wide_matches([{"routeId":"real-route-40"},
                    {"routeId":"real-route-40"}], self.routes, ["B 40"])), 1)

    def test_invalid_line_label(self):
        with self.assertRaises(ValueError):
            normalize_display_line("Tram 7 (maybe)")


if __name__ == "__main__":
    unittest.main()
