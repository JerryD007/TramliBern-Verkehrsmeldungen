from pathlib import Path
import subprocess,tempfile,re
r=Path(__file__).resolve().parents[1]
source=(r/'RundeUhr/RundeUhr.ino').read_text()
union=source[source.index('union ReceiveScratch'):source.index('int actualView')]
pre='#include <new>\n#include <type_traits>\n#include <cassert>\n#include <cstdio>\n'
for name in ['Finance','Aircraft','Iss','Quake','Launch','News']:
 pre+=f'#include "{name}Model.h"\n'
 pre+=re.search(r'struct '+name+r'Result \{.*?\};',(r/f'RundeUhr/{name}Network.h').read_text()).group(0)+'\n'
clock=(r/'RundeUhr/ClockNetwork.h').read_text()
for name in ['NetDiagnostics','NetResult']:
 pre+=re.search(r'struct '+name+r' \{.*?\};',clock).group(0)+'\n'
post='''int main(){
 constexpr size_t old=sizeof(NetResult)+sizeof(FinanceResult)+sizeof(NewsResult)+sizeof(LaunchResult)+sizeof(QuakeResult)+sizeof(IssResult)+sizeof(AircraftResult);
 static_assert(old-sizeof(ReceiveScratch)>4000,"Expected saved receive workspace");
 for(int i=0;i<10000;i++){
 auto& n=*new (&receiveScratch.news) NewsResult;n.data.count=3;strcpy(n.data.items[2].title,"Test");assert(!strcmp(n.data.items[2].title,"Test"));
 auto& w=*new (&receiveScratch.weather) NetResult;assert(!w.weather.valid&&w.weather.count==0);w.weather.temps[71]=42;
 auto& a=*new (&receiveScratch.aircraft) AircraftResult;assert(a.config.aircraftRadius==50);
 }
 printf("PASS receive scratch lifetimes, host sizes %zu -> %zu; saved %zu bytes (ESP sizes verified by compiler)\\n",old,sizeof(ReceiveScratch),old-sizeof(ReceiveScratch));}
'''
with tempfile.TemporaryDirectory() as t:
 f=Path(t)/'test.cpp';f.write_text(pre+union+post)
 subprocess.run(['g++','-std=c++17','-fsanitize=address,undefined','-I',str(r/'RundeUhr'),str(f),'-o',t+'/test'],check=True)
 subprocess.run([t+'/test'],check=True)
