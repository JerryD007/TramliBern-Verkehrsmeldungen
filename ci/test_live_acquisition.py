"""Exercise download ordering/provenance without network or live credentials."""
import json
import os
from pathlib import Path
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch
from live_gtfs_diagnostic import fetch_inputs, STATIC_URL, SA_URL


class LiveAcquisition(unittest.TestCase):
    def test_static_first_and_original_timestamp_retained(self):
        calls = []
        class Response:
            headers = {'Date': 'fresh server date', 'Age': '0'}
            content = b'original binary'
            url = 'https://example.test/static.zip?temporary=secret'
            def raise_for_status(self): pass
            def iter_content(self, _): yield b'archive'
            def __enter__(self): return self
            def __exit__(self, *args): pass
        def get(url, **kwargs):
            calls.append(url)
            return Response()
        payload = {'header': {'gtfsRealtimeVersion': '2.0',
                              'incrementality': 'FULL_DATASET', 'timestamp': 99},
                   'entity': []}
        modules = {'requests': SimpleNamespace(Session=lambda: SimpleNamespace(get=get)),
                   'protobuf_decode': SimpleNamespace(protobuf_payload=lambda _: payload)}
        old = Path.cwd()
        try:
            with tempfile.TemporaryDirectory() as folder, patch.dict(sys.modules, modules), \
                    patch('time.time', return_value=1000):
                os.chdir(folder)
                alerts, archive, meta = fetch_inputs('test-placeholder', Path(folder))
                self.assertEqual(calls, [STATIC_URL, SA_URL])
                self.assertEqual(meta['source_timestamp'], 99)
                self.assertIsNotNone(meta['freshness_error'])
                self.assertEqual(meta['resolved_url'], 'https://example.test/static.zip')
                self.assertEqual(archive.read_bytes(), b'archive')
                self.assertEqual(Path('reports/real-gtfs-source.pb').read_bytes(), b'original binary')
                evidence = json.loads(Path('reports/source-observation.json').read_text())
                self.assertEqual(evidence['source_clock_status'], 'stale')
        finally:
            os.chdir(old)
