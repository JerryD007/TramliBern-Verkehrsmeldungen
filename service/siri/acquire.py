"""Bounded HTTPS acquisition, separate from the lightweight HTTP request path."""
import logging
import os
import threading
import time
import urllib.request
from .app import application

MIRROR = 'https://tools.opentransportdata.swiss/data/siri-sx/siri_sx-latest-prod.xml'
FULL = 'https://api.opentransportdata.swiss/la/siri-sx'
UNPLANNED = 'https://api.opentransportdata.swiss/la/siri-sx-unplanned'


class NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, *args, **kwargs):
        return None  # Never forward an API credential to another host.


def refresh(app, url, token=None):
    if url not in (MIRROR, FULL, UNPLANNED):
        raise ValueError('source not allowed')
    headers = {'Accept':'application/xml','User-Agent':'TramliBern-SIRI-Test/0.1'}
    if token and url in (FULL, UNPLANNED):
        headers['Authorization'] = 'Bearer ' + token
    request = urllib.request.Request(url,headers=headers)
    with urllib.request.build_opener(NoRedirect()).open(request,timeout=30) as stream:
        return app.load(stream)


def start(app, url, interval, token=None):
    # Mirror refresh/quota is undocumented: conservative hourly research mode.
    minimum = 60 if url == UNPLANNED else (1800 if url == FULL else 3600)
    if url not in (MIRROR,FULL,UNPLANNED) or interval < minimum:
        raise ValueError('unsupported source or polling interval')
    def run():
        while True:
            try:
                refresh(app,url,token)
            except Exception:
                logging.warning('SIRI refresh failed; no source body or credentials logged')
            time.sleep(interval)
    thread = threading.Thread(target=run,daemon=True)
    thread.start()
    return thread


# A single acquisition worker/process is required to respect source quotas.
if os.environ.get('SIRI_SOURCE_URL'):
    start(application,os.environ['SIRI_SOURCE_URL'],int(os.environ.get('SIRI_INTERVAL','3600')),
          os.environ.get('OPENTRANSPORT_API_TOKEN'))
