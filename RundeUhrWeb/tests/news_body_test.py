from pathlib import Path
import subprocess,tempfile
r=Path(__file__).resolve().parents[1]
source=r'''#include <cassert>
#include <cstring>
#include <string>
#include <algorithm>
#include <stdint.h>
uint32_t clockMs=0;
uint32_t millis(){return clockMs;}
void delay(unsigned n){clockMs+=n;}
#include "NewsBody.h"
struct Input{
 std::string bytes;size_t pos=0;bool live=true,stall=false;int packet=37;
 int available(){return stall?0:std::min(size_t(packet),bytes.size()-pos);}
 bool connected(){return live;}
 int read(uint8_t*b,int n){n=std::min(size_t(n),bytes.size()-pos);memcpy(b,bytes.data()+pos,n);pos+=n;return n;}
};
struct Sink{std::string bytes;bool fail=false;size_t write(uint8_t*b,size_t n){if(fail)return 0;bytes.append((char*)b,n);return n;}};
int main(){
 Input i;i.bytes=std::string(35974,'x');Sink s;assert(readNewsBody(i,s,35974)==35974&&s.bytes==i.bytes);
 i=Input();s=Sink();i.bytes="partial";i.live=false;assert(readNewsBody(i,s,100)==-5);
 i=Input();s=Sink();i.stall=true;clockMs=0xfffffff0U;assert(readNewsBody(i,s,100)==-11);
 i=Input();s=Sink();i.bytes="abc";s.fail=true;assert(readNewsBody(i,s,3)==-10);
 i=Input();s=Sink();assert(readNewsBody(i,s,262145)==-10&&s.bytes.empty());
 i.bytes="abcEXTRA";assert(readNewsBody(i,s,3)==3&&s.bytes=="abc"&&i.pos==3);
 puts("PASS RSS bounded read: 35974 bytes, fragmentation, disconnect, timeout/wrap, sink rejection, size limit, exact body length");
}'''
with tempfile.TemporaryDirectory() as d:
 p=Path(d)/'test.cpp';p.write_text(source)
 subprocess.run(['g++','-std=c++17','-fsanitize=address,undefined','-I',str(r/'RundeUhr'),str(p),'-o',d+'/test'],check=True)
 subprocess.run([d+'/test'],check=True)
