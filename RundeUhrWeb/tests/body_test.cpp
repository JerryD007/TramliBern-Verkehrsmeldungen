#include <cassert>
#include <string>
#include <cstdio>
#include <ArduinoJson.h>
#include <cstdlib>
static size_t allocated=0,peak=0,limit=40000;
void* trackedMalloc(size_t n){if(allocated+n>limit)return nullptr;auto p=(size_t*)std::malloc(n+sizeof(size_t));if(!p)return nullptr;*p=n;allocated+=n;if(allocated>peak)peak=allocated;return p+1;}
void trackedFree(void*p){if(!p)return;auto h=(size_t*)p-1;allocated-=*h;std::free(h);}
#define malloc trackedMalloc
#define free trackedFree
#include "../RundeUhr/BoundedBody.h"
#undef malloc
#undef free
int main(){
 for(int repeat=0;repeat<500;repeat++){BoundedBody b(32768);std::string payload(30000,'x');for(size_t i=0;i<payload.size();i+=713){size_t k=std::min(size_t(713),payload.size()-i);assert(b.write((const uint8_t*)payload.data()+i,k)==k);}assert(b.size()==30000);for(char c:payload)assert(b.read()==c);assert(b.read()==-1&&b.available()==0);}
 assert(allocated==0&&peak<32000);limit=1024;{BoundedBody f(4096);std::string z(2000,'x');assert(f.write((const uint8_t*)z.data(),z.size())<z.size()&&f.allocationFailed());}assert(allocated==0);limit=40000;
 BoundedBody b(5);assert(b.write((const uint8_t*)"hello",5)==5);assert(b.write('!')==0&&b.overflowed());
 BoundedBody j(1024);const char*s="{\"value\":123,\"text\":\"München\"}";assert(j.write((const uint8_t*)s,strlen(s))==strlen(s));JsonDocument d;assert(!deserializeJson(d,j));assert(d["value"]==123);assert(!strcmp(d["text"],"München"));
 puts("PASS segmented body: 500 x 30KB, boundaries, overflow, JSON UTF-8 and release");
}
