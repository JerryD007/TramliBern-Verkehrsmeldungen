from pathlib import Path
import subprocess,tempfile
r=Path(__file__).resolve().parents[1];s=(r/'RundeUhr/LaunchNetwork.h').read_text();fn=s[s.index('inline void launchTick'):]
pre='''#include "LaunchModel.h"
#include "FinanceModel.h"
#include <cassert>
bool otaActive=false,networkBusy=false,memoryOK=true,fail=false;uint32_t clockMs=1,calls=0;int launchResultQueue=0;
struct LaunchResult{LaunchData data;};LaunchResult output;
uint32_t millis(){return clockMs;}
bool financeMemoryReady(Quote&){return memoryOK;}
void fetchLaunch(LaunchData&a,uint32_t&retry){assert(networkBusy&&!otaActive);calls++;if(fail){strcpy(a.error,"HTTP 429");retry=7200000;return;}a.valid=true;a.count=1;a.error[0]=0;}
void xQueueOverwrite(int,LaunchResult*r){output=*r;}
'''
post='''int main(){Config c;launchTick(c);assert(calls==0);c.view=15;launchTick(c);assert(calls==1&&!networkBusy);clockMs+=1799999;launchTick(c);assert(calls==1);clockMs++;launchTick(c);assert(calls==2);otaActive=true;clockMs+=1800000;launchTick(c);assert(calls==2&&!networkBusy);otaActive=false;memoryOK=false;launchTick(c);assert(calls==2&&output.data.valid);memoryOK=true;clockMs+=14999;launchTick(c);assert(calls==2);clockMs++;fail=true;launchTick(c);assert(calls==3&&output.data.valid);c.view=0;c.mask=32769;clockMs+=7199999;launchTick(c);assert(calls==3);clockMs++;fail=false;launchTick(c);assert(calls==4);c.mask=1;clockMs+=1800000;launchTick(c);assert(calls==4);c.view=15;launchTick(c);assert(calls==5);puts("PASS launch scheduler: inactive/manual/cycle15, 30min throttle, OTA/RAM gates, cache retained, switching cannot bypass 429 backoff.");}'''
with tempfile.TemporaryDirectory() as t:
 p=Path(t)/'test.cpp';p.write_text(pre+fn+post);subprocess.run(['g++','-std=c++17','-I',str(r/'RundeUhr'),str(p),'-o',t+'/test'],check=True);subprocess.run([t+'/test'],check=True)
