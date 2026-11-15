#include <Adafruit_GFX.h>
#include "../RundeUhr/Render.h"
#include "../RundeUhr/ConfigCodec.h"
#include <cassert>
#include <fstream>
int main(){for(int m=1;m<8192;m++){auto n=mergedScreenMask(m);assert(n&&!(n&20));assert((n&~30)==(m&~30));assert(bool(n&8)==bool(m&24));assert(bool(n&2)==bool(m&6));assert(mergedScreenMask(n)==n);for(int v=0;v<13;v++){int next=nextView(v,m);assert(next!=2&&next!=4&&(n&(1<<next)));}}for(int v=0;v<13;v++)assert(mergedScreenView(v)==(v==4?3:v==2?1:v));
 JsonDocument d;deserializeJson(d,R"({"schema":4,"city":"Bern","lat":46.948,"lon":7.4474,"event":"Ferien","date":"","view":4,"mask":16,"interval":5,"automatic":true,"ssid":"saved","pass":"secret","countdownType":0,"animations":true,"customEvent":"Event","countdownDates":["","2027-03-25","","2027-05-31",""]})");Config c;bool migrated;assert(loadConfig(d,c,migrated)&&migrated&&c.view==3&&c.mask==8&&!strcmp(c.pass,"secret"));assert(loadConfig(d,c,migrated)&&!migrated&&c.view==3&&c.mask==8);d["view"]=12;d["mask"]=4096;assert(loadConfig(d,c,migrated)&&!migrated&&c.view==12&&c.mask==4096);
 setenv("TZ",ZURICH_TZ,1);tzset();time_t now;assert(parseDate("2026-11-15",now));now+=12*3600;Weather w;w.valid=true;w.days=1;w.day[0]=now;w.sunrise[0]=now-5*3600;w.sunset[0]=now+5*3600;Moon moon;moon.valid=true;moon.full=now+9*86400;GFXcanvas8 g(240,240);c.view=3;
 for(double phase:{0.,.22,.25,.5,.75,.99}){moon.phase=phase;moon.lit=(1-cos(phase*2*M_PI))*50;for(int missing=0;missing<3;missing++){render(g,c,missing==1?Weather():w,missing==2?Moon():moon,now,true,false,"","");for(int i=0;i<57600;i++)if((i%240-120)*(i%240-120)+(i/240-120)*(i/240-120)>119*119)assert(g.getBuffer()[i]==BG);}}
 d["view"]=2;d["mask"]=4;assert(loadConfig(d,c,migrated)&&migrated&&c.view==1&&c.mask==2);assert(loadConfig(d,c,migrated)&&!migrated);assert(!strcmp(c.pass,"secret"));
 c.view=1;w.count=3;w.received=now;for(int k=0;k<3;k++){w.hours[k]=now+(k+1)*3600;w.temps[k]=12+k;w.probability[k]=20+k*30;w.amount[k]=k*.6;}
 for(int fields=0;fields<16;fields++)for(int count=0;count<=3;count++)for(int code:{0,2,45,61,75,95}){w.count=count;for(int k=0;k<3;k++){w.fields[k]=fields;w.codes[k]=code;}render(g,c,w,moon,now,true,false,"","");for(int i=0;i<57600;i++)if((i%240-120)*(i%240-120)+(i/240-120)*(i/240-120)>119*119)assert(g.getBuffer()[i]==BG);}
 puts("PASS merged weather/rain: 16 field combinations, 0-3 hours, six weather codes, preserved credentials and idempotent migration.");
 puts("PASS merged Sun/Moon: all 8191 masks x 13 previous selections, OR migration, idempotence, independent screen IDs, credentials, six lunar phases, missing solar/lunar data, circular clipping.");}
