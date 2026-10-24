"""Build a short-lived v2 test feed from explicitly verified stop selectors only."""
import json
from alert_decode import integer, active_periods

MAX_BYTES=22000

def text_of(value):
    if not isinstance(value,dict): return ''
    translations=value.get('translation',[])
    if not isinstance(translations,list): return ''
    valid=[t for t in translations if isinstance(t,dict) and isinstance(t.get('text'),str) and t['text'].strip()]
    german=[t for t in valid if t.get('language','').lower().startswith('de')]
    return ((german or valid)[0]['text'].strip() if valid else '')


def build_feed(report, alerts, now, source_timestamp=None):
    # Source records have already passed strict full-dataset timestamp validation.
    # Missing provenance must never make an old feed appear freshly generated.
    if type(source_timestamp) is not int or not now-900 < source_timestamp <= now+120:
        raise ValueError('source no longer fresh')
    expiry=min(now+900,source_timestamp+900)
    by_id={a['id']:a for a in alerts}
    rows=[]
    seen=set()
    for match in report['matches']:
        if match['status']!='explicit_stop': continue
        alert=by_id[match['alert_id']]
        if not active_periods(alert.get('activePeriod',[]),now): continue
        text=text_of(alert.get('headerText')) or text_of(alert.get('descriptionText'))
        if not text: continue
        selected=report['selected_stops'][match['stop']]
        key=(alert['id'],selected)
        if key in seen: continue
        seen.add(key)
        end=expiry
        for p in alert.get('activePeriod',[]):
            start=integer(p['start']) if 'start' in p else 0
            stop=integer(p['end']) if 'end' in p else None
            if start <= now and (stop is None or now < stop):
                end=min(end,stop if stop is not None else end)
        rows.append({'id':alert['id'],'stops':[selected],'text':text[:180],
                     'start':now,'end':end,'verification':'explicit_stop'})
    feed={'schema':2,'generated':now,'expires':expiry,'alerts':rows,
          'static_sha256':report['static']['sha256'], 'source_timestamp':source_timestamp}
    # Fail rather than silently discarding potentially relevant entries.
    if len(rows)>32 or len(json.dumps(feed,ensure_ascii=False).encode())>MAX_BYTES:
        raise ValueError('verified feed exceeds device capacity')
    return feed


def build_line_feed(report, alerts, now, source_timestamp):
    # Reuse expiry/text limits, but give the new semantic contract its own endpoint.
    converted = dict(report, matches=[dict(m, status='explicit_stop')
                                     for m in report.get('line_matches', [])])
    rows = []
    seen = set()
    for match in converted['matches']:
        key = (match['alert_id'], match['stop'], match['route_id'])
        if key in seen: continue
        seen.add(key)
        part = build_feed(dict(converted, matches=[match]), alerts, now, source_timestamp)
        for row in part['alerts']:
            row.update(verification='line_relevance', scope='line',
                       route_id=match['route_id'], line=match['line'])
            prefix = 'Linienmeldung ' + match['line'] + ': '
            row['text'] = (prefix + row['text'])[:180]
            rows.append(row)
    feed = build_feed(dict(report, matches=[]), alerts, now, source_timestamp)
    feed.update(schema=3, alerts=rows)
    if len(rows)>32 or len(json.dumps(feed,ensure_ascii=False).encode())>MAX_BYTES:
        raise ValueError('line feed exceeds device capacity')
    return feed
