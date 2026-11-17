#include <cassert>
#include <string>
#include <cstdio>
#include <ArduinoJson.h>
#include "../RundeUhr/BoundedBody.h"
int main(){
 for(int repeat=0;repeat<500;repeat++){BoundedBody b(32768);std::string payload(30000,'x');for(size_t i=0;i<payload.size();i+=713){size_t k=std::min(size_t(713),payload.size()-i);assert(b.write((const uint8_t*)payload.data()+i,k)==k);}assert(b.size()==30000);for(char c:payload)assert(b.read()==c);assert(b.read()==-1&&b.available()==0);}
 BoundedBody b(5);assert(b.write((const uint8_t*)"hello",5)==5);assert(b.write('!')==0&&b.overflowed());
 BoundedBody j(1024);const char*s="{\"value\":123,\"text\":\"München\"}";assert(j.write((const uint8_t*)s,strlen(s))==strlen(s));JsonDocument d;assert(!deserializeJson(d,j));assert(d["value"]==123);assert(!strcmp(d["text"],"München"));
 puts("PASS segmented body: 500 x 30KB, boundaries, overflow, JSON UTF-8 and release");
}
