import hashlib
import unittest
from source_observation import observe_source


class SourceObservation(unittest.TestCase):
    def test_fresh_http_does_not_refresh_old_content(self):
        r = observe_source(b'old', {'Date': 'now', 'Age': '0'}, 100, 1000)
        self.assertEqual(r['source_clock_status'], 'stale')
        self.assertEqual(r['source_timestamp'], 100)
        self.assertEqual(r['sha256'], hashlib.sha256(b'old').hexdigest())

    def test_future_and_expiry_boundaries(self):
        for stamp, status in ((100, 'stale'), (101, 'within_policy'),
                              (1120, 'within_policy'), (1121, 'future')):
            with self.subTest(stamp=stamp):
                self.assertEqual(observe_source(b'', {}, stamp, 1000)
                                 ['source_clock_status'], status)

    def test_only_safe_headers_are_archived_case_insensitively(self):
        r = observe_source(b'', {'DaTe': 'x', 'Set-Cookie': 'secret',
                                'Authorization': 'secret', 'X-Token': 'secret'}, 1000, 1000)
        self.assertEqual(r['http_headers'], {'date': 'x'})

    def test_boolean_clock_rejected(self):
        with self.assertRaises(ValueError):
            observe_source(b'', {}, True, 1000)
