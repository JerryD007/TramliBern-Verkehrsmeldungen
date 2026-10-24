import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch
import test_static_integrity
from live_gtfs_diagnostic import crosscheck

class DiagnosticIntegration(unittest.TestCase):
    def test_counts_and_unknown_ids_are_visible(self):
        fixture=test_static_integrity.StaticIntegrity().data()
        with tempfile.TemporaryDirectory() as root:
            path=Path(root)/'gtfs.zip';path.write_bytes(fixture)
            with patch('live_gtfs_diagnostic.SELECTED',{'S':'station'}), patch('live_gtfs_diagnostic.Path',side_effect=lambda p:Path(root)/p):
                r=crosscheck([{'id':'a','selectors':[{'routeId':'missing'}, {'routeId':'r'}, {'stopId':'p'}]}],path,{'sha256':'fixture'})
            self.assertEqual(r['counts'],dict(explicit_stop=1,route_candidate=1,not_matched=0,unverifiable=1))
            self.assertEqual(r['compatibility']['routeId']['found'],1)
            self.assertEqual(r['selected_evidence']['S']['serving_routes'],['r'])
            self.assertTrue((Path(root)/'reports/real-gtfs-selected.zip').exists())
