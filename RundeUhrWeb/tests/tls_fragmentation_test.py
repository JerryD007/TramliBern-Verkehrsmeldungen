from pathlib import Path
import subprocess,tempfile
r=Path(__file__).resolve().parents[1]
s=(r/'RundeUhr/FinanceNetwork.h').read_text();fn=s[s.index('inline bool financeMemoryReady'):s.index('// Called by')]
pre=r'''#include "FinanceModel.h"
#include <cassert>
#include <vector>
#include <algorithm>
#include <cstdio>
#define RUNDE_TLS_INPUT_BYTES 16717
#define RUNDE_TLS_OUTPUT_BYTES 16717
#define MALLOC_CAP_8BIT 1
std::vector<size_t> blocks;unsigned outstanding=0;size_t used[8]={};
unsigned heap_caps_get_largest_free_block(int){return *std::max_element(blocks.begin(),blocks.end());}
struct E{unsigned getFreeHeap(){unsigned n=0;for(auto b:blocks)n+=b;return n;}}ESP;
void* alloc(size_t n){for(size_t i=0;i<blocks.size();i++)if(blocks[i]>=n){blocks[i]-=n;used[i]=n;outstanding++;return reinterpret_cast<void*>(i+1);}return nullptr;}
void release(void*p){if(!p)return;size_t i=reinterpret_cast<size_t>(p)-1;blocks[i]+=used[i];used[i]=0;outstanding--;}
#define malloc alloc
#define free release
'''
post=r'''int main(){Quote q;blocks={20468,17000,13000,13000,14540};auto original=blocks;assert(ESP.getFreeHeap()==78008);void*a=alloc(18432);void*b=alloc(18432);assert(a&&!b);release(a);assert(blocks==original);assert(financeMemoryReady(q));assert(q.memoryGate==0&&q.largestAfterFirst==17000&&outstanding==0&&blocks==original);
blocks={20468,16000,14000,14000,14000};original=blocks;assert(!financeMemoryReady(q));assert(q.memoryGate==3&&q.largestAfterFirst==16000&&outstanding==0&&blocks==original);
blocks={16000,16000,16000,16000,16000};original=blocks;assert(!financeMemoryReady(q)&&q.memoryGate==2&&blocks==original);
blocks={20000,20000,22832};original=blocks;assert(!financeMemoryReady(q)&&q.memoryGate==1&&q.heapBefore==62832&&blocks==original);
puts("PASS synthetic fragmentation: RC4 false rejection vs actual TLS sizes; true second-buffer shortage, first-buffer shortage and total-heap floor still rejected; all probes freed. Synthetic blocks are not a measured heap map.");}
'''
with tempfile.TemporaryDirectory() as t:
 p=Path(t)/'test.cpp';p.write_text(pre+fn+post);subprocess.run(['g++','-std=c++17','-I',str(r/'RundeUhr'),str(p),'-o',t+'/test'],check=True);subprocess.run([t+'/test'],check=True)
