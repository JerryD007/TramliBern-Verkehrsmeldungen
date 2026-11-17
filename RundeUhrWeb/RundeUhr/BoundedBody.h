#pragma once
#include <Stream.h>
#include <stdlib.h>
#include <string.h>
// Fixed small blocks: never realloc/copy the whole response while TLS is alive.
// Consumed blocks are released as the JSON parser reads, after TLS is stopped.
class BoundedBody : public Stream {
 struct Block {Block* next;size_t used;uint8_t data[512];};
 Block *head=nullptr,*tail=nullptr;size_t maximum,used=0,readCount=0,offset=0;
 bool overflow=false,failed=false;
 public:
 explicit BoundedBody(size_t limit):maximum(limit){setTimeout(0);}
 ~BoundedBody(){while(head){auto b=head;head=b->next;free(b);}}
 BoundedBody(const BoundedBody&)=delete;BoundedBody& operator=(const BoundedBody&)=delete;
 bool ready()const{return true;}bool overflowed()const{return overflow;}bool allocationFailed()const{return failed;}
 size_t size()const{return used;}
 size_t write(uint8_t c)override{return write(&c,1);}
 size_t write(const uint8_t*src,size_t n)override{
  if(n>maximum-used){overflow=true;return 0;}size_t done=0;
  while(done<n){if(!tail||tail->used==512){auto b=static_cast<Block*>(malloc(sizeof(Block)));if(!b){failed=true;return done;}b->next=nullptr;b->used=0;if(tail)tail->next=b;else head=b;tail=b;}
   size_t k=n-done;if(k>512-tail->used)k=512-tail->used;memcpy(tail->data+tail->used,src+done,k);tail->used+=k;used+=k;done+=k;
  }return done;
 }
 int available()override{return int(used-readCount);}
 int peek()override{return head?head->data[offset]:-1;}
 int read()override{if(!head)return -1;int c=head->data[offset++];readCount++;if(offset==head->used){auto b=head;head=b->next;if(!head)tail=nullptr;free(b);offset=0;}return c;}
 size_t readBytes(char* dst,size_t n){size_t k=0;while(k<n&&head)dst[k++]=char(read());return k;}
 void flush()override{}
};
