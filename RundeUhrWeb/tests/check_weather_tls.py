from pathlib import Path
import re,subprocess,tempfile
r=Path(__file__).resolve().parents[1]
roots=re.findall(r'R"PEM\((.*?)\)PEM"',(r/'RundeUhr/Trust.h').read_text(),re.S)
assert len(roots)==2
url='https://api.open-meteo.com/v1/forecast?latitude=46.948&longitude=7.4474&hourly=temperature_2m,weather_code,precipitation_probability,precipitation&daily=sunrise,sunset&timezone=Europe%2FZurich&timeformat=unixtime&forecast_days=3'
with tempfile.TemporaryDirectory() as t:
 for i,pem in enumerate(roots):
  ca=Path(t)/'ca.pem';ca.write_text(pem)
  result=subprocess.run(['curl','--fail','--max-time','35','--cacert',str(ca),url,'-o',str(r/'tests/live_weather.json')])
  if result.returncode==0:
   print('PASS Open-Meteo HTTPS with shipped trust profile',i,flush=True);break
 else:raise RuntimeError('Weather HTTPS failed for both shipped trust profiles')
subprocess.run([str(r/'build/weather_live_test'),str(r/'tests/live_weather.json')],check=True)
