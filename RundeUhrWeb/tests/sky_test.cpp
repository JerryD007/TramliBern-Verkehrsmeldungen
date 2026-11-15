#include <Adafruit_GFX.h>
#include "../RundeUhr/Render.h"
#include "../RundeUhr/ConfigCodec.h"
#include <cassert>
#include <fstream>
int main(){for(int m=1;m<8192;m++){auto n=mergedSkyMask(m);assert(n&&!(n&16));assert((n&~24)==(m&~24));assert(bool(n&8)==bool(m&24));assert(mergedSkyMask(n)==n);for(int v=0;v<13;v++){int next=nextView(v,m);assert(next!=4&&(n&(1<<next)));}}for(int v=0;v<13;v++)assert(mergedSkyView(v)==(v==4?3:v));
 JsonDocument d;deserializeJson(d,R"({"schema":4,"city":"Bern","lat":46.948,"lon":7.4474,"event":"Ferien","date":"","view":4,"mask":16,"interval":5,"automatic":true,"ssid":"saved","pass":"secret","countdownType":0,"animations":true,"customEvent":"Event","countdownDates":["","2027-03-25","","2027-05-31",""]})");Config c;bool migrated;assert(loadConfig(d,c,migrated)&&migrated&&c.view==3&&c.mask==8&&!strcmp(c.pass,"secret"));assert(loadConfig(d,c,migrated)&&!migrated&&c.view==3&&c.mask==8);d["view"]=12;d["mask"]=4096;assert(loadConfig(d,c,migrated)&&!migrated&&c.view==12&&c.mask==4096);
 setenv("TZ",ZURICH_TZ,1);tzset();time_t now;assert(parseDate("2026-11-15",now));now+=12*3600;Weather w;w.valid=true;w.days=1;w.day[0]=now;w.sunrise[0]=now-5*3600;w.sunset[0]=now+5*3600;Moon moon;moon.valid=true;moon.full=now+9*86400;GFXcanvas8 g(240,240);c.view=3;
 for(double phase:{0.,.22,.25,.5,.75,.99}){moon.phase=phase;moon.lit=(1-cos(phase*2*M_PI))*50;for(int missing=0;missing<3;missing++){render(g,c,missing==1?Weather():w,missing==2?Moon():moon,now,true,false,"","");for(int i=0;i<57600;i++)if((i%240-120)*(i%240-120)+(i/240-120)*(i/240-120)>119*119)assert(g.getBuffer()[i]==BG);}}
 puts("PASS merged Sun/Moon: all 8191 masks x 13 previous selections, OR migration, idempotence, independent screen IDs, credentials, six lunar phases, missing solar/lunar data, circular clipping.");}
