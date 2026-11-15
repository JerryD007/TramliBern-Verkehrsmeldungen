from pathlib import Path
import subprocess,tempfile
p=Path(__file__).resolve().parents[1]
s=(p/'RundeUhr/ClockNetwork.h').read_text();body=s[s.index('inline bool fetchWeather'):s.index('#include "FinanceNetwork.h"')]
pre=r'''
#include "Arduino.h"
#include <cassert>
#include <vector>
#include <ctime>
struct Config{double lat=46.948,lon=7.4474;};struct Weather{};
struct JsonDocument{void clear(){}};
struct NetDiagnostics{uint8_t trustProfile=0;int http=0,tls=0,bytes=0;};
const char*TRUST_CURRENT="current";const char*TRUST_LEGACY="legacy";
std::vector<int> calls;int scenario=0;
unsigned millis(){return 0;}
bool requestJSON(String url,JsonDocument&,String&error,NetDiagnostics&d,const char*root){
 assert(url.find("https://api.open-meteo.com/")==0);calls.push_back(root==TRUST_CURRENT?0:1);
 if(scenario==0&&root==TRUST_CURRENT){d.tls=-12288;error="certificate";return false;}
 if(scenario==1){d.tls=-32512;error="RAM";return false;}
 if(scenario==2){d.http=429;error="rate limit";return false;}
 if(scenario==3){d.tls=-9984;error="certificate";return false;}
 if(scenario==4){d.http=200;error="JSON";return false;}
 return true;
}
bool parseWeather(JsonDocument&,Weather&,String&,time_t,unsigned){return scenario!=5;}
'''
post=r'''
int main(int argc,char**argv){scenario=atoi(argv[1]);Config c;Weather w;String error;NetDiagnostics d;
 bool result=fetchWeather(c,w,error,d);
 if(scenario==0){assert(result);assert((calls==std::vector<int>{0,1}));calls.clear();assert(fetchWeather(c,w,error,d));assert((calls==std::vector<int>{1}));}
 else if(scenario==3){assert(!result);assert(calls.size()==2);}
 else{assert(!result);assert(calls.size()==1);}
}
'''
with tempfile.TemporaryDirectory() as t:
 f=Path(t)/'test.cpp';f.write_text(pre+body+post)
 subprocess.run(['g++','-std=c++17','-I',str(p/'tests/host'),str(f),'-o',t+'/test'],check=True)
 for case in range(6):subprocess.run([t+'/test',str(case)],check=True)
print('PASS: actual fetchWeather trust fallback; remember successful profile; max two tries; no retry for RAM, rate limit, JSON or weather parse errors. Host stubs, not a TLS handshake.')
