"""Read-only live API/schema smoke tests. No printer credentials, no hardware tests."""
import urllib.request,json,xml.etree.ElementTree as ET,time,concurrent.futures
URLS={'iss':'https://api.wheretheiss.at/v1/satellites/25544','quake':'https://earthquake.usgs.gov/earthquakes/feed/v1.0/summary/4.5_day.geojson','launch_dev':'https://lldev.thespacedevs.com/2.3.0/launches/upcoming/?limit=1&mode=list','rss':'https://www.tagesschau.de/infoservices/alle-meldungen-100~rss2.xml'}
def check(kv):
 name,url=kv;t=time.monotonic()
 with urllib.request.urlopen(urllib.request.Request(url,headers={'User-Agent':'RundeUhr-development/RC17'}),timeout=30) as r:
  data=r.read(500001);assert len(data)<=500000;assert r.status==200
 if name=='rss':
  root=ET.fromstring(data);items=root.findall('./channel/item');assert items and all(x.findtext('title') for x in items);count=len(items)
 else:
  d=json.loads(data)
  if name=='iss':assert d['id']==25544 and d['units']=='kilometers' and -90<=d['latitude']<=90 and -180<=d['longitude']<=180;count=1
  elif name=='quake':assert d['type']=='FeatureCollection';count=len(d['features']);assert all(len(x['geometry']['coordinates'])==3 and x['properties']['time']>0 for x in d['features'])
  else:count=len(d['results']);assert count and all(x['name'] and 'net' in x and 'status' in x for x in d['results'])
 return {'name':name,'url':url,'http':200,'bytes':len(data),'items':count,'seconds':round(time.monotonic()-t,2),'schema':'PASS','development_data':name=='launch_dev'}
if __name__=='__main__':
 results=list(concurrent.futures.ThreadPoolExecutor(4).map(check,URLS.items()));print(json.dumps(results,indent=2))
