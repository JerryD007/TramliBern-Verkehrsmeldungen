from pathlib import Path
import subprocess,tempfile,os
r=Path(__file__).resolve().parents[1]
s=(r/'RundeUhr/AircraftRouteNetwork.h').read_text();fn=s[s.index('inline void enrichAircraftRoute'):]
pre='''#include "AircraftRoute.h"
#include "FinanceModel.h"
#include <cassert>
bool otaActive=false,memoryOK=true;unsigned ms=0,calls=0;int response=200;unsigned millis(){return ms;}time_t fakeTime(void*){return 1000;}
bool financeMemoryReady(Quote&){return memoryOK;}
int fetchAircraftRoute(const char*,char*o,char*d,uint32_t&delay){calls++;if(response==200){strcpy(o,"Start City");strcpy(d,"Test City");}else o[0]=d[0]=0;if(response==429)delay=600000;return response;}
#define time fakeTime
'''
post='''int main(){AircraftData a;a.checked=a.found=true;a.received=a.positionAt=1000;a.error[0]=0;strcpy(a.callsign,"TEST123");otaActive=true;enrichAircraftRoute(a);assert(!calls&&!a.origin[0]&&!a.destination[0]);otaActive=false;memoryOK=false;enrichAircraftRoute(a);assert(!calls);memoryOK=true;enrichAircraftRoute(a);assert(calls==1&&!strcmp(a.destination,"Test City")&&!strcmp(a.origin,"Start City")&&a.error[0]==0);enrichAircraftRoute(a);assert(calls==1);strcpy(a.callsign,"OTHER123");enrichAircraftRoute(a);assert(calls==1&&!a.origin[0]&&!a.destination[0]);ms=60000;response=404;enrichAircraftRoute(a);assert(calls==2&&!a.origin[0]&&!a.destination[0]&&a.found&&!a.error[0]);ms+=60000;enrichAircraftRoute(a);assert(calls==2);strcpy(a.callsign,"THIRD123");response=429;enrichAircraftRoute(a);assert(calls==3&&!a.origin[0]&&!a.destination[0]&&a.found&&!a.error[0]);strcpy(a.callsign,"FOURTH123");ms+=60000;enrichAircraftRoute(a);assert(calls==3);ms+=540000;response=200;enrichAircraftRoute(a);assert(calls==4&&a.destination[0]);a.found=false;enrichAircraftRoute(a);assert(!a.origin[0]&&!a.destination[0]&&calls==4);puts("PASS actual route enrichment: OTA/RAM gates, positive/negative cache, changed callsign never reuses old destination, 60s global throttle, HTTP429 backoff, route errors preserve position, empty detection clears destination.");}'''
with tempfile.TemporaryDirectory() as t:
 p=Path(t)/'test.cpp';p.write_text(pre+fn+post)
 subprocess.run(['g++','-std=c++17','-I',str(r/'RundeUhr'),'-I',os.environ['JSON_DIR'],str(p),'-o',t+'/test'],check=True)
 subprocess.run([t+'/test'],check=True)
