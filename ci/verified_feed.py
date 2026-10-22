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


def build_feed(report, alerts, now):
    # Source records have already passed strict full-dataset timestamp validation.
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
        end=now+900
        for p in alert.get('activePeriod',[]):
            start=integer(p['start']) if 'start' in p else 0
            stop=integer(p['end']) if 'end' in p else None
            if start <= now and (stop is None or now < stop):
                end=min(end,stop if stop is not None else end)
        rows.append({'id':alert['id'],'stops':[selected],'text':text[:180],
                     'start':now,'end':end,'verification':'explicit_stop'})
    feed={'schema':2,'generated':now,'expires':now+900,'alerts':rows,
          'static_sha256':report['static']['sha256']}
    # Fail rather than silently discarding potentially relevant entries.
    if len(rows)>32 or len(json.dumps(feed,ensure_ascii=False).encode())>MAX_BYTES:
        raise ValueError('verified feed exceeds device capacity')
    return feed
