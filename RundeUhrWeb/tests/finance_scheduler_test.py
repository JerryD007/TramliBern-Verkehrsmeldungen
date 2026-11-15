from pathlib import Path
import tempfile,subprocess
r=Path(__file__).resolve().parents[1]
s=(r/'RundeUhr/FinanceNetwork.h').read_text();fn=s[s.index('inline bool financeMemoryReady'):]
pre='''#include "FinanceModel.h"
#define RUNDE_TLS_INPUT_BYTES 16717
#define RUNDE_TLS_OUTPUT_BYTES 16717
#include <cassert>
#include <cstdio>
bool otaActive=false,networkBusy=false;unsigned clockMs=1,calls=0,freeHeap=90000,largest=24000,allocations=0,outstanding=0,failAllocation=0;int financeResultQueue=0;
struct FinanceResult {Config config;FinanceData data;};FinanceResult output;
unsigned millis(){return clockMs;}struct Esp {unsigned getFreeHeap(){return freeHeap;}} ESP;
#define MALLOC_CAP_8BIT 1
unsigned heap_caps_get_largest_free_block(int){return largest;}
void* probeMalloc(size_t n){assert(n==16717);allocations++;if(failAllocation&&allocations==failAllocation)return nullptr;outstanding++;return ::malloc(n);}
void probeFree(void*p){if(p)outstanding--;::free(p);}
#define malloc probeMalloc
#define free probeFree
void xQueueOverwrite(int,FinanceResult*r){output=*r;}
void fetchQuote(Quote&q,const char*,const char*,bool){assert(networkBusy&&!otaActive);calls++;q.valid=true;q.value=123;q.received=clockMs/1000;q.error[0]=0;}
'''
post='''int main(){Config c;financeTick(c,false);assert(calls==0);c.view=10;financeTick(c,false);assert(calls==1&&output.data.btc.valid&&!networkBusy);clockMs+=119999;financeTick(c,false);assert(calls==1);clockMs++;financeTick(c,false);assert(calls==2);otaActive=true;clockMs+=120000;financeTick(c,false);assert(calls==2&&!networkBusy);otaActive=false;c.view=0;financeTick(c,false);assert(calls==2);c.view=11;for(int i=0;i<4;i++)financeTick(c,false);assert(calls==5);c.view=10;strcpy(c.btcCurrency,"EUR");financeTick(c,false);assert(calls==6&&!strcmp(output.config.btcCurrency,"EUR"));clockMs+=120000;freeHeap=50000;financeTick(c,false);assert(calls==6&&output.data.btc.valid&&output.data.btc.error[0]&&!networkBusy);freeHeap=90000,largest=24000,allocations=0,outstanding=0,failAllocation=0;clockMs+=14999;financeTick(c,false);assert(calls==6);clockMs++;financeTick(c,false);assert(calls==7&&!output.data.btc.error[0]);assert(output.data.btc.heapBefore==90000&&output.data.btc.largestBefore==24000&&outstanding==0);clockMs+=120000;failAllocation=allocations+2;financeTick(c,false);assert(calls==7&&output.data.btc.memoryDeferred&&outstanding==0);failAllocation=0;clockMs+=15000;financeTick(c,false);assert(calls==8&&!output.data.btc.memoryDeferred);clockMs+=120000;failAllocation=allocations+1;financeTick(c,false);assert(calls==8&&output.data.btc.memoryDeferred&&outstanding==0);puts("PASS finance scheduling: inactive modules, 120-second refresh, 15-second RAM retry, exact-size two-buffer allocation, cleanup on first/second allocation failure, heap diagnostics, pair rotation, setting change, OTA gate, low RAM deferral, cached value retention.");}
'''
with tempfile.TemporaryDirectory() as t:
 p=Path(t)/'test.cpp';p.write_text(pre+fn+post);subprocess.run(['g++','-std=c++17','-I',str(r/'RundeUhr'),str(p),'-o',t+'/test'],check=True);subprocess.run([t+'/test'],check=True)
