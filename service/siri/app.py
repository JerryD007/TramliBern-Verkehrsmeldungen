"""WSGI prototype. Cache load is explicit, off the ESP32 request path."""
import json
import os
import threading
import time
from urllib.parse import parse_qs
from .core import parse_stream, response


class Application:
    def __init__(self):
        self.snapshot = None
        self.lock = threading.Lock()

    def load(self, stream):
        candidate = parse_stream(stream)
        # Failed loads never replace a previously complete snapshot.
        with self.lock:
            self.snapshot = candidate
        return candidate

    def __call__(self, environ, start_response):
        status = '200 OK'
        try:
            if environ.get('REQUEST_METHOD') != 'GET':
                status, result = '405 Method Not Allowed', {'error':'GET required'}
            elif environ.get('PATH_INFO') != '/v1/line-messages':
                status, result = '404 Not Found', {'error':'unknown endpoint'}
            else:
                q = parse_qs(environ.get('QUERY_STRING',''), keep_blank_values=True, max_num_fields=4)
                if set(q) != {'operator_ref','line_ref'} or any(len(v)!=1 or not v[0] or len(v[0])>160 for v in q.values()):
                    status, result = '400 Bad Request', {'error':'exact operator_ref and line_ref required'}
                elif self.snapshot is None:
                    status, result = '503 Service Unavailable', {'error':'source unavailable','alerts':[]}
                else:
                    result = response(self.snapshot,int(time.time()),q['operator_ref'][0],q['line_ref'][0])
        except ValueError:
            status, result = '503 Service Unavailable', {'error':'source or request invalid','alerts':[]}
        body = json.dumps(result,ensure_ascii=False,separators=(',',':')).encode()
        if len(body)>22000:
            status, body = '503 Service Unavailable', b'{"error":"response too large","alerts":[]}'
        start_response(status,[('Content-Type','application/json; charset=utf-8'),('Content-Length',str(len(body))),('Cache-Control','no-store')])
        return [body]


application = Application()
# Offline/replay mode for independent deployment and contract tests. Production
# acquisition must atomically load a fresh snapshot from a supervised fetcher.
if os.environ.get('SIRI_SNAPSHOT_PATH'):
    with open(os.environ['SIRI_SNAPSHOT_PATH'],'rb') as source:
        application.load(source)

if __name__ == '__main__':
    from wsgiref.simple_server import make_server
    with make_server('127.0.0.1', int(os.environ.get('PORT','8080')), application) as server:
        server.serve_forever()
