#pragma once
#include <stdint.h>
// Identity transfer with a known Content-Length: no HTTPClient heap buffer.
// Stream is the TLS client's buffered input; sink is the transactional RSS parser.
template<class Input,class Sink> int readNewsBody(Input& input,Sink& sink,int expected){
 if(expected<0||expected>262144)return -10;
 uint8_t buffer[256];int received=0;uint32_t started=millis(),last=started;
 while(received<expected){
  int available=input.available();
  if(available>0){
   int want=expected-received;if(want>256)want=256;if(want>available)want=available;
   int got=input.read(buffer,want);
   if(got>0){if(sink.write(buffer,got)!=size_t(got))return -10;received+=got;last=millis();}
  }else if(!input.connected())return -5;
  if(uint32_t(millis()-last)>=10000||uint32_t(millis()-started)>=45000)return -11;
  delay(1);
 }
 return received;
}
