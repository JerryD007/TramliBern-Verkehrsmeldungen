#pragma once
#include <Stream.h>
#include <stdlib.h>
#include <string.h>
// Grow with the actual response instead of reserving 48 KiB during TLS.
class BoundedBody : public Stream {
 public:
  explicit BoundedBody(size_t limit):maximum(limit){capacity=limit<2048?limit:2048;data=static_cast<char*>(malloc(capacity+1));if(data)data[0]=0;}
  ~BoundedBody(){free(data);}
  BoundedBody(const BoundedBody&)=delete;BoundedBody& operator=(const BoundedBody&)=delete;
  bool ready()const{return data!=nullptr;}
  bool overflowed()const{return overflow;}
  bool allocationFailed()const{return failed;}
  const char* bytes()const{return data;}size_t size()const{return used;}
  size_t write(uint8_t c)override{return write(&c,1);}
  size_t write(const uint8_t*src,size_t n)override{
   if(!data)return 0;if(n>maximum-used){overflow=true;return 0;}
   if(used+n>capacity){size_t next=(used+n+1023)/1024*1024;if(next>maximum)next=maximum;char* bigger=static_cast<char*>(realloc(data,next+1));if(!bigger){failed=true;return 0;}data=bigger;capacity=next;}
   memcpy(data+used,src,n);used+=n;data[used]=0;return n;
  }
  int available()override{return 0;}int read()override{return -1;}int peek()override{return -1;}void flush()override{}
 private:size_t maximum,capacity=0,used=0;char*data=nullptr;bool overflow=false,failed=false;
};
