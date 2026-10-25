import io
import json
import threading
import unittest
import urllib.request
from unittest.mock import patch
from wsgiref.simple_server import make_server,WSGIRequestHandler
from test_siri_state import source,NOW
from test_siri_service import OP,LINE
from service.siri.runtime import run_source
from service.siri.state import FeedState
from service.siri.app import Application
from service.siri.acquire import FULL,UNPLANNED

class Stop:
    def __init__(self): self.delays=[]
    def is_set(self): return bool(self.delays)
    def wait(self,delay): self.delays.append(delay)

class Quiet(WSGIRequestHandler):
    def log_message(self,*args): pass

class RuntimeTests(unittest.TestCase):
    def run_kind(self,kind,broken=False):
        state=FeedState();stop=Stop();calls=[]
        def fetch(url,token):
            calls.append(url)
            if broken: raise ValueError('bad source')
            return source()
        with self.assertLogs(level='WARNING') if broken else patch('service.siri.runtime.logging.warning'):
            run_source(state,kind,'test-only',stop,fetch,lambda:NOW)
        return stop.delays,calls
    def test_planned_interval(self): self.assertEqual(self.run_kind('full'),([43200],[FULL]))
    def test_unplanned_interval(self): self.assertEqual(self.run_kind('unplanned'),([60],[UNPLANNED]))
    def test_full_retry_not_tight_loop(self): self.assertEqual(self.run_kind('full',True)[0],[1800])
    def test_realtime_retry(self): self.assertEqual(self.run_kind('unplanned',True)[0],[60])
    def test_http_end_to_end_historical_replay(self):
        state=FeedState();state.ingest('full',source(),NOW)
        state.ingest('unplanned',source(present=False),NOW)
        with make_server('127.0.0.1',0,Application(state),handler_class=Quiet) as server:
            thread=threading.Thread(target=server.handle_request);thread.start()
            with patch('service.siri.app.time.time',return_value=NOW+60):
                with urllib.request.urlopen(f'http://127.0.0.1:{server.server_port}/v1/line-messages?operator_ref={OP}&line_ref={LINE}',timeout=5) as r:
                    self.assertEqual(r.status,200)
                    self.assertEqual(r.headers['Cache-Control'],'no-store')
                    raw=r.read();self.assertLess(len(raw),22000)
                    result=json.loads(raw)
                    self.assertEqual(len(result['alerts']),1)
                    self.assertEqual(result['full_source_timestamp'],NOW)
                    self.assertEqual(result['unplanned_source_timestamp'],NOW)
            thread.join(timeout=5);self.assertFalse(thread.is_alive())
