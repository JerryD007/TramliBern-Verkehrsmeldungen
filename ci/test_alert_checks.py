"""Offline regression tests: no token, network access or ESP32 is required."""
import json
import unittest

from alert_checks import (
    MAX_BYTES, matching_alerts, sloid_matches, validate_feed,
    verified_route_match,
)

NOW = 1800000000
STOP = "ch:1:sloid:87480"
OTHER = "ch:1:sloid:87481"


def feed(alerts, generated=NOW):
    return json.dumps({"generated": generated, "alerts": alerts}, ensure_ascii=False).encode("utf-8")


def alert(stops=None, *, text="Linie 7 unterbrochen", end=NOW + 100):
    return {"stops": [STOP] if stops is None else stops, "text": text, "end": end}


class JsonContractTests(unittest.TestCase):
    def test_valid_existing_format(self):
        parsed = validate_feed(feed([alert()]), now=NOW)
        self.assertEqual(len(parsed["alerts"]), 1)

    def test_empty_feed_is_valid(self):
        self.assertEqual(validate_feed(feed([]))["alerts"], [])

    def test_invalid_json_fails_closed(self):
        with self.assertRaises(ValueError):
            validate_feed(b"{broken")

    def test_rejects_missing_or_empty_stop_ids(self):
        for ids in ([], ["not-a-sloid"], ["ch:1:sloid:87480oops"], [42]):
            with self.subTest(ids=ids), self.assertRaises(ValueError):
                validate_feed(feed([alert(ids)]))

    def test_rejects_missing_text_and_bad_time(self):
        for entry in (alert(text=" "), {**alert(), "end": "tomorrow"}):
            with self.subTest(entry=entry), self.assertRaises(ValueError):
                validate_feed(feed([entry]))

    def test_rejects_oversized_json(self):
        with self.assertRaises(ValueError):
            validate_feed(b" " * (MAX_BYTES + 1))

    def test_rejects_malformed_timestamp(self):
        with self.assertRaises(ValueError):
            validate_feed(feed([], generated="bad"))

    def test_utf8_german_text(self):
        original = "Störung: Bus fällt aus"
        self.assertEqual(validate_feed(feed([alert(text=original)]))["alerts"][0]["text"], original)


class MatchingTests(unittest.TestCase):
    def test_selected_station_matches(self):
        f = validate_feed(feed([alert()]))
        self.assertEqual(len(matching_alerts(f, [STOP], now=NOW)), 1)

    def test_different_station_does_not_match(self):
        f = validate_feed(feed([alert()]))
        self.assertEqual(matching_alerts(f, [OTHER], now=NOW), [])

    def test_platform_sloid_matches_station(self):
        f = validate_feed(feed([alert([STOP + ":0:1"])]))
        self.assertEqual(len(matching_alerts(f, [STOP], now=NOW)), 1)

    def test_prefix_collision_does_not_match(self):
        self.assertFalse(sloid_matches("ch:1:sloid:874801", STOP))

    def test_expired_alert_is_suppressed(self):
        f = validate_feed(feed([alert(end=NOW - 1)]))
        self.assertEqual(matching_alerts(f, [STOP], now=NOW), [])

    def test_end_zero_is_unbounded(self):
        f = validate_feed(feed([alert(end=0)]))
        self.assertEqual(len(matching_alerts(f, [STOP], now=NOW)), 1)

    def test_no_selected_stops_yields_nothing(self):
        f = validate_feed(feed([alert()]))
        self.assertEqual(matching_alerts(f, [], now=NOW), [])

    def test_route_id_does_not_magically_match_line(self):
        self.assertFalse(verified_route_match(["91-9-j26-1"], ["9"]))
        self.assertFalse(verified_route_match(["91-9-j26-1"], ["91-9-j26-1"]))
        self.assertTrue(verified_route_match(["91-9-j26-1"], ["91-9-j26-1"], verified_mapping=True))
        self.assertFalse(verified_route_match(["91-9-j26-1"], ["92-9-j26-1"], verified_mapping=True))


if __name__ == "__main__":
    unittest.main()
