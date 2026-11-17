from pathlib import Path
import subprocess,xml.etree.ElementTree as E,email.utils,datetime,json
root=Path(__file__).resolve().parents[1]
feeds=[("ct24","https://ct24.ceskatelevize.cz/rss/tema/vyber-redakce-84313",False),("cnn","https://news.google.com/rss/search?q=site%3Acnn.com%20when%3A1d&hl=en-US&gl=US&ceid=US%3Aen",True)]
report=[];failed=False
for name,url,cnn in feeds:
 p=root/"build"/("live_"+name+".xml")
 subprocess.run(["curl","--fail","--retry","2","--max-time","40","--cacert",str(root/"certificates/finance_roots.pem"),url,"-o",str(p)],check=True)
 items=E.fromstring(p.read_bytes()).findall("./channel/item")
 if cnn: items=[i for i in items if i.findtext("source")=="CNN"]
 assert items, name+" has no items"
 dates=[email.utils.parsedate_to_datetime(i.findtext("pubDate")) for i in items]
 newest=max(dates);age=(datetime.datetime.now(datetime.timezone.utc)-newest).total_seconds()
 assert -600<=age<172800,(name,"stale/future",newest,age)
 result=subprocess.run([str(root/"build/news_test"),str(p)]+(["cnn"] if cnn else []));failed=failed or result.returncode!=0
 report.append(dict(source=name,url=url,bytes=p.stat().st_size,items=len(items),newest=newest.isoformat(),age_seconds=age,tls="host curl with shipped roots; not ESP32"))
 print("Live source",report[-1],"parser",result.returncode,flush=True)
(root/"build/news_live_report.json").write_text(json.dumps(report,indent=2))

assert not failed,"Firmware RSS parser rejected a live feed"
