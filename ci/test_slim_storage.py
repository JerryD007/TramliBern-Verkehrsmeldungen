"""Compile and execute the actual firmware storage functions with fake NVS."""
from pathlib import Path
import subprocess,tempfile
s=Path('firmware/TramliBern/TramliBern.ino').read_text()
def function(name):
    import re
    m=re.search(r'(?:String|void|int) '+name+r'\([^)]*\)\s*\{',s);assert m,name
    start=m.start();pos=m.end();level=1
    while level:
        if s[pos]=='{': level+=1
        if s[pos]=='}': level-=1
        pos+=1
    return s[start:pos]
code=r'''
#include <string>
#include <map>
#include <cassert>
#include <cstdint>
using std::string;
struct String : string {
 using string::string;
 String()=default;
 String(const string& s):string(s){}
 String(int n):string(std::to_string(n)){}
};
struct FakePrefs {
 std::map<string,String> strings; std::map<string,int> numbers;
 uint8_t getUChar(const char* k,int d){return numbers.count(k)?numbers[k]:d;}
 void putUChar(const char*k,uint8_t v){numbers[k]=v;}
 bool getBool(const char*k,bool d){return numbers.count(k)?numbers[k]:d;}
 void putBool(const char*k,bool v){numbers[k]=v;}
 String getString(const char*k,const char*d){return strings.count(k)?strings[k]:String(d);}
 void putString(const char*k,String v){strings[k]=v;}
} prefs;
constexpr int MAX_STOPS=5,PAGE_COUNT=3,STOPS_PER_PAGE=2,CITY_COUNT=13;
int lastRefresh=123;
int activeCity=0,activePage=0,stopCount=0;
String stops[MAX_STOPS];
void resetStopData(){}
'''
for name in ['cityStopKey','cityCountKey','cityPageKey','restoreCityPage','selectPage','loadCityStops','saveStops','firstFreeSlotOnPage','removeStopAt','cityAtTouch']:code+=function(name)+'\n'
code+=r'''
int main(){
 // Every visible city opens its own index; gaps and blank cells do nothing.
 for(int city=0;city<CITY_COUNT;++city){
   int x=12+(city%2)*234, y=43+(city/2)*37;
   assert(cityAtTouch(x,y)==city);
   assert(cityAtTouch(x+221,y+32)==city);
   assert(cityAtTouch(x+222,y+16)==-1);
   assert(cityAtTouch(x+110,y+33)==-1);
 }
 assert(cityAtTouch(300,280)==-1);
 assert(cityAtTouch(20,20)==-1);
 assert(cityAtTouch(-1,80)==-1);
 assert(cityAtTouch(480,80)==-1);
 assert(cityAtTouch(20,320)==-1);

 // Empty board: the selected page supplies the default, never page 1.
 assert(firstFreeSlotOnPage(0)==0);
 assert(firstFreeSlotOnPage(1)==2);
 assert(firstFreeSlotOnPage(2)==4);
 assert(firstFreeSlotOnPage(-1)==-1 && firstFreeSlotOnPage(3)==-1);
 stops[2]="A";assert(firstFreeSlotOnPage(1)==3);
 stops[3]="B";assert(firstFreeSlotOnPage(1)==-1);
 stops[4]="C";assert(firstFreeSlotOnPage(2)==-1);
 for(auto &s:stops)s="";

 // Legacy settings migrate without changing their slots.
 prefs.putUChar("count",2);prefs.putString("stop0","A");prefs.putString("stop1","B");
 loadCityStops();assert(stopCount==2 && stops[0]=="A" && stops[1]=="B");
 // Configure page 3 only, persist, erase RAM, restore.
 for(auto &s:stops)s="";stops[4]="Paul Klee";saveStops();selectPage(2);
 for(auto &s:stops)s="";activePage=0;loadCityStops();restoreCityPage();
 assert(stopCount==1 && stops[4]=="Paul Klee" && stops[0].empty() && activePage==2);
 // A second city has independent data and remembered page.
 activeCity=1;loadCityStops();restoreCityPage();assert(stopCount==0 && activePage==0);
 stops[2]="Zurich";saveStops();selectPage(1);
 activeCity=0;loadCityStops();restoreCityPage();assert(activePage==2 && stops[4]=="Paul Klee");
 activeCity=1;loadCityStops();restoreCityPage();assert(activePage==1 && stops[2]=="Zurich");
 // Delete one slot without moving another.
 stops[4]="Other";saveStops();removeStopAt(2);loadCityStops();
 assert(stopCount==1 && stops[2].empty() && stops[4]=="Other" && activePage==1 && lastRefresh==0);
 removeStopAt(-1);removeStopAt(5);assert(stopCount==1 && stops[4]=="Other");
 removeStopAt(4);loadCityStops();assert(stopCount==0 && firstFreeSlotOnPage(2)==4);
 prefs.putUChar(cityPageKey(1).c_str(),99);restoreCityPage();assert(activePage==0);
 selectPage(-1);selectPage(3);assert(activePage==0);
}
'''
with tempfile.TemporaryDirectory() as d:
 p=Path(d);(p/'test.cpp').write_text(code)
 subprocess.run(['g++','-std=c++17',str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
for forbidden in ['ALERTS_BASE','serviceAlertsTick','alerts-status','mappedSloid','AlertSafety.h']:
 assert forbidden not in s,forbidden
assert 'selectPage((activePage + 1) % PAGE_COUNT)' in s
assert 'selectPage(next)' in s
assert 'restoreCityPage();' in s
print('PASS: real storage functions: legacy migration, sparse slots, city isolation, restart, deletion, page bounds; alert removal')
