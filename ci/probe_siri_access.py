"""Read-only check of existing API entitlement; never prints credentials/bodies."""
import json
import os
from pathlib import Path
import sys
import time
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from service.siri.acquire import FULL,UNPLANNED,NoRedirect
from service.siri.core import parse_stream
import urllib.request
import urllib.error

results=[]
for url in (FULL,UNPLANNED):
    token=os.environ.get('OPENTRANSPORT_API_TOKEN','').strip()
    item={'source':url,'checked_at':int(time.time()),'token_configured':bool(token)}
    if token:
        request=urllib.request.Request(url,headers={'Authorization':'Bearer '+token,'Accept':'application/xml','User-Agent':'TramliBern-SIRI-Entitlement-Test/0.1'})
        try:
            with urllib.request.build_opener(NoRedirect()).open(request,timeout=90) as reply:
                item['http_status']=reply.status
                snapshot=parse_stream(reply)
                item.update({k:v for k,v in snapshot.items() if k in ('source_timestamp','situations','bytes','oversized_situations')})
                item['supported_rows']=len(snapshot['rows'])
                item['source_age_seconds']=int(time.time())-snapshot['source_timestamp']
        except urllib.error.HTTPError as e: item['http_status']=e.code
        except Exception as e: item['error_type']=type(e).__name__
    results.append(item)
Path('reports').mkdir(exist_ok=True)
Path('reports/siri-access.json').write_text(json.dumps(results,indent=2))
print(json.dumps(results,indent=2))
if not all(r.get('http_status')==200 and 'source_timestamp' in r for r in results):
    raise SystemExit('SIRI entitlement or payload not verified; no firmware switch')
