"""Recheck expiry and device contract immediately before publishing a test feed."""
import json
import re
import sys
import time
from pathlib import Path
from live_gtfs_diagnostic import SELECTED
from verified_feed import MAX_BYTES


def check_feed(data, now):
    if len(data) > MAX_BYTES:
        raise ValueError('feed too large')
    f = json.loads(data)
    if not isinstance(f, dict) or f.get('schema') not in (2, 3):
        raise ValueError('wrong feed schema')
    for name in ('generated', 'expires', 'source_timestamp'):
        if type(f.get(name)) is not int:
            raise ValueError('missing/invalid feed timestamp')
    if not now - 900 < f['source_timestamp'] <= now + 120:
        raise ValueError('source expired while preparing publication')
    if not now - 900 < f['generated'] <= now + 120:
        raise ValueError('generation time invalid')
    if not now < f['expires'] <= min(f['source_timestamp'] + 900, f['generated'] + 900):
        raise ValueError('expiry outside source lifetime')
    if not isinstance(f.get('static_sha256'), str) or not re.fullmatch('[0-9a-f]{64}', f['static_sha256']):
        raise ValueError('missing static provenance')
    alerts = f.get('alerts')
    if not isinstance(alerts, list) or len(alerts) > 32:
        raise ValueError('invalid alert list')
    for a in alerts:
        expected = 'line_relevance' if f['schema'] == 3 else 'explicit_stop'
        if not isinstance(a, dict) or a.get('verification') != expected:
            raise ValueError('unverified alert')
        if f['schema'] == 3:
            if (a.get('scope') != 'line' or not isinstance(a.get('route_id'), str) or not a['route_id']
                or not isinstance(a.get('line'), str) or not a['line']
                or not isinstance(a.get('text'), str)
                or not a['text'].startswith('Linienmeldung ' + a['line'] + ': ')):
                raise ValueError('missing line scope or label')
        if not isinstance(a.get('id'), str) or not a['id']:
            raise ValueError('missing alert identity')
        if not isinstance(a.get('text'), str) or not a['text'].strip() or len(a['text']) > 180:
            raise ValueError('invalid warning text')
        if not isinstance(a.get('stops'), list) or len(a['stops']) != 1 or a['stops'][0] not in SELECTED.values():
            raise ValueError('stop outside verified test coverage')
        if type(a.get('start')) is not int or type(a.get('end')) is not int or not 0 < a['start'] <= now < a['end'] <= f['expires']:
            raise ValueError('expired/invalid alert')
    return f


if __name__ == '__main__':
    check_feed(Path(sys.argv[1]).read_bytes(), int(time.time()))
    print('Test feed contract and remaining source lifetime verified.')
