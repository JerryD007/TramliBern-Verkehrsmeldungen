from pathlib import Path
import subprocess,tempfile
r=Path(__file__).resolve().parents[1];s=(r/'RundeUhr/AircraftNetwork.h').read_text();fn=s[s.index('inline void aircraftTick'):]
pre='''#include "AircraftModel.h"
#include "FinanceModel.h"
#include <cassert>
bool otaActive=false,networkBusy=false,memoryOK=true,fail=false;unsigned clockMs=1,calls=0;int aircraftResultQueue=0;
struct AircraftResult{Config config;AircraftData data;};AircraftResult output;
unsigned millis(){return clockMs;}
bool financeMemoryReady(Quote&q){q.heapBefore=90000;q.largestBefore=24000;return memoryOK;}
void fetchAircraft(AircraftData&a,const Config&,uint32_t&retry){assert(networkBusy&&!otaActive);calls++;if(fail){strcpy(a.error,"HTTP 429");retry=600000;return;}a.checked=a.found=true;a.received=clockMs/1000;a.error[0]=0;}
void xQueueOverwrite(int,AircraftResult*r){output=*r;}
'''
post='''int main(){Config c;aircraftTick(c);assert(calls==0);c.view=12;aircraftTick(c);assert(calls==1&&!networkBusy);clockMs+=29999;aircraftTick(c);assert(calls==1);clockMs++;aircraftTick(c);assert(calls==2);otaActive=true;clockMs+=30000;aircraftTick(c);assert(calls==2&&!networkBusy);otaActive=false;memoryOK=false;aircraftTick(c);assert(calls==2&&output.data.memoryDeferred&&output.data.found);memoryOK=true;clockMs+=14999;aircraftTick(c);assert(calls==2);clockMs++;aircraftTick(c);assert(calls==3);c.aircraftLat=50;aircraftTick(c);assert(calls==3);clockMs+=30000;fail=true;aircraftTick(c);assert(calls==4&&!output.data.found);clockMs+=599999;aircraftTick(c);assert(calls==4);clockMs++;fail=false;aircraftTick(c);assert(calls==5);c.view=0;clockMs+=30000;aircraftTick(c);assert(calls==5);c.mask|=4096;aircraftTick(c);assert(calls==6);puts("PASS aircraft scheduling: inactive/cycle/manual, 30s interval, OTA gate, 15s RAM retry, cached position, config changes cannot bypass backoff, HTTP429 Retry-After.");}'''
with tempfile.TemporaryDirectory() as t:
 p=Path(t)/'test.cpp';p.write_text(pre+fn+post);subprocess.run(['g++','-std=c++17','-I',str(r/'RundeUhr'),str(p),'-o',t+'/test'],check=True);subprocess.run([t+'/test'],check=True)
