"""Read only a protobuf feed header to diagnose JSON freshness failures.

This is NOT an alert decoder and cannot publish warnings. Preserve raw evidence.
"""
import json,os,time
from pathlib import Path

def varint(data,pos):
    value=0
    for shift in range(0,70,7):
        if pos>=len(data): raise ValueError('truncated varint')
        b=data[pos];pos+=1;value|=(b&127)<<shift
        if b<128:return value,pos
    raise ValueError('oversized varint')

def fields(data):
    pos=0
    while pos<len(data):
        key,pos=varint(data,pos);field,wire=key>>3,key&7
        if field==0:raise ValueError('invalid field')
        if wire==0:value,pos=varint(data,pos)
        elif wire in (1,2,5):
            if wire==2:size,pos=varint(data,pos)
            else:size=8 if wire==1 else 4
            if pos+size>len(data):raise ValueError('truncated field')
            value=data[pos:pos+size];pos+=size
        else:raise ValueError('unsupported wire type')
        yield field,wire,value

def header_timestamp(data):
    headers=[v for n,w,v in fields(data) if n==1 and w==2]
    if len(headers)!=1:raise ValueError('missing/duplicate header')
    stamps=[v for n,w,v in fields(headers[0]) if n==3 and w==0]
    if len(stamps)!=1:raise ValueError('missing/duplicate timestamp')
    return stamps[0]

def main():
    import requests
    root=Path('reports');root.mkdir(exist_ok=True)
    response=requests.get('https://api.opentransportdata.swiss/la/gtfs-sa',headers={'Authorization':'Bearer '+os.environ['OPENTRANSPORT_API_TOKEN'],'Cache-Control':'no-cache'},timeout=120)
    response.raise_for_status()
    (root/'protobuf-freshness-probe.pb').write_bytes(response.content)
    stamp=header_timestamp(response.content);now=int(time.time())
    result={'diagnostic_only':True,'timestamp':stamp,'fetched_at':now,'age_seconds':now-stamp,'fresh':now-900<stamp<=now+120}
    from source_observation import observe_source
    result['acquisition'] = observe_source(response.content, response.headers, stamp, now)
    (root/'protobuf-freshness-probe.json').write_text(json.dumps(result,indent=2))
    print(json.dumps(result))
if __name__=='__main__':main()
