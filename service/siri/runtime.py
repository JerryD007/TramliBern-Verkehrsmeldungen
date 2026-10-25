"""Production-shaped two-source runtime. Not a claim of hosting or API entitlement.

One process, one pair of acquisition threads, no network I/O in HTTP handlers.
"""
import logging
import os
import threading
import time
from urllib.request import Request,build_opener
from .acquire import FULL,UNPLANNED,NoRedirect
from .core import parse_stream
from .state import FeedState
from .app import Application


def fetch(url,token):
    req=Request(url,headers={'Authorization':'Bearer '+token,'Accept':'application/xml',
                             'User-Agent':'TramliBern-SIRI-Service/0.2'})
    with build_opener(NoRedirect()).open(req,timeout=30) as reply:
        return parse_stream(reply)


def run_source(state,kind,token,stop,fetcher=fetch,clock=time.time):
    url,interval=(FULL,43200) if kind=='full' else (UNPLANNED,60)
    while not stop.is_set():
        try:
            state.ingest(kind,fetcher(url,token),int(clock()))
            delay=interval
        except Exception:
            # Failed full downloads must not cause an API quota storm.
            delay=1800 if kind=='full' else 60
            logging.warning('SIRI %s acquisition failed',kind)
        stop.wait(delay)


state=FeedState()
application=Application(state)
stop=threading.Event()
# Gunicorn imports the module once in ONE worker (do not use --preload).
token=os.environ.get('OPENTRANSPORT_API_TOKEN','').strip()
if token:
    for kind in ('full','unplanned'):
        threading.Thread(target=run_source,args=(state,kind,token,stop),daemon=True).start()
