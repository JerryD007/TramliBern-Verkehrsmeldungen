import unittest
from probe_protobuf import header_timestamp
class HeaderProbe(unittest.TestCase):
    def test_header(self):self.assertEqual(header_timestamp(b'\x0a\x02\x18\x2a'),42)
    def test_truncated(self):
        with self.assertRaises(ValueError):header_timestamp(b'\x0a\x02\x18')
    def test_duplicate(self):
        with self.assertRaises(ValueError):header_timestamp(b'\x0a\x02\x18\x2a'*2)
