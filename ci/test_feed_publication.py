import json
import unittest
from check_feed_publication import check_feed


class PublicationSafety(unittest.TestCase):
    def feed(self):
        return dict(schema=2, generated=1000, expires=1800, source_timestamp=900,
                    static_sha256='a'*64, alerts=[])
    def check(self, f, now=1000): return check_feed(json.dumps(f).encode(), now)
    def test_empty_verified_feed_allowed(self): self.check(self.feed())
    def test_delay_after_build_blocks_publication(self):
        with self.assertRaises(ValueError): self.check(self.feed(), 1800)
    def test_missing_provenance(self):
        f=self.feed();del f['source_timestamp']
        with self.assertRaises(ValueError): self.check(f)
    def test_expiry_cannot_be_extended(self):
        f=self.feed();f['expires']=1801
        with self.assertRaises(ValueError): self.check(f)
    def test_candidate_never_published(self):
        f=self.feed();f['alerts']=[{'verification':'candidate'}]
        with self.assertRaises(ValueError): self.check(f)
    def test_control_station_never_published(self):
        f=self.feed();f['alerts']=[dict(id='control',text='Moved',verification='explicit_stop',
                                      stops=['ch:1:sloid:90035'],start=1000,end=1800)]
        with self.assertRaises(ValueError): self.check(f)
