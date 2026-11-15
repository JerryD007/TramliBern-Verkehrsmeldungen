#include <string>
using String=std::string;
#include "../RundeUhr/ParseData.h"
#include "../RundeUhr/Moon.h"
#include <cassert>
#include <iostream>
#include <fstream>
int main(){setenv("TZ",ZURICH_TZ,1);tzset();
 Config c;JsonDocument d;bool migrated;
 const char* legacy=R"({"schema":3,"city":"Bern","lat":46.948,"lon":7.4474,"event":"Ferien","date":"2027-01-01","interval":15,"view":5,"mask":63,"automatic":true,"ssid":"SavedWiFi","pass":"secret123","countdownType":0,"countdownDates":["2027-01-01","2027-01-02","2027-01-03","2027-01-04","2027-01-05"],"customEvent":"Pruefung","animations":true})";
 for(int type=0;type<5;type++)for(int view=0;view<6;view++)for(int mask=1;mask<=63;mask++){
  deserializeJson(d,legacy);d["view"]=view;d["mask"]=mask;d["countdownType"]=type;
  assert(loadConfig(d,c,migrated)&&migrated);assert(c.view==mergedScreenView(view==5?5+type:view));assert(c.mask==mergedScreenMask((mask&31)|((mask&32)?1<<(5+type):0)));
  assert(!strcmp(c.ssid,"SavedWiFi")&&!strcmp(c.pass,"secret123"));assert(!strcmp(c.countdownDates[4],"2027-01-05"));
  assert(loadConfig(d,c,migrated)&&!migrated);
 }
 for(int schema=1;schema<=2;schema++){deserializeJson(d,legacy);d["schema"]=schema;d["view"]=schema==1?9:5;d["mask"]=schema==1?512:32;assert(loadConfig(d,c,migrated)&&migrated);assert(c.view==9&&c.mask==512);assert(!strcmp(c.countdownDates[4],"2027-01-01"));}
 for(int mask=1;mask<=1023;mask++)for(int view=0;view<10;view++){int next=nextView(view,mask);assert(next>=0&&next<10&&(mergedScreenMask(mask)&(1<<next)));}
 deserializeJson(d,legacy);d["schema"]=4;
 for(int view=0;view<10;view++){d["view"]=view;d["mask"]=1023;assert(decodeConfig(d,c)&&c.view==mergedScreenView(view));}
 for(int k=0;k<5;k++){d["countdownType"]=k;assert(decodeConfig(d,c));assert(!strcmp(c.date,c.countdownDates[k]));}
 Config good=c;d["view"]=VIEW_COUNT;assert(!decodeConfig(d,c));d["view"]=9;d["mask"]=0;assert(!decodeConfig(d,c));d["mask"]=1023;d["countdownDates"][0]="2027-02-30";assert(!decodeConfig(d,c));assert(!strcmp(c.countdownDates[0],good.countdownDates[0]));d["countdownDates"][0]="2027-01-01";d["animations"]=false;assert(decodeConfig(d,c)&&!c.animations);
 d["deviceName"]="Kuchyň";assert(decodeConfig(d,c));assert(loadConfig(d,c,migrated)&&!strcmp(c.deviceName,"Kuchyň"));d["deviceName"]="Bad\nName";assert(!decodeConfig(d,c));
 time_t a,b;assert(!parseDate("2026-02-29",a));assert(parseDate("2028-02-29",a));assert(!parseDate("2026-2-003",a));assert(parseDate("2026-03-29",a));assert(parseDate("2026-03-30",b));assert(b-a==23*3600&&calendarDays(b,a)==1);assert(parseDate("2026-10-25",a));assert(parseDate("2026-10-26",b));assert(b-a==25*3600&&calendarDays(b,a)==1);
 std::ifstream f("tests/weather_fixture.json");JsonDocument data;assert(!deserializeJson(data,f));Weather w;String err;time_t now=data["hourly"]["time"][1].as<int64_t>();assert(parseWeather(data,w,err,now,100));assert(nextHour(w,now)==2);assert(!weatherStale(w,now));assert(weatherStale(w,now+1801));assert(nextHour(w,w.hours[w.count-1])==-1);auto saved=w;data["hourly"]["precipitation"][1]=nullptr;assert(parseWeather(data,w,err,now,100));assert(!(w.fields[1]&8)&&w.fields[1]&1);assert(sunDay(w,now)>=0);assert(w.sunrise[sunDay(w,now)]!=0);
 // Missing rain/symbol/temperature does not erase independent sun products.
 data.remove("hourly");assert(parseWeather(data,w,err,now,100));assert(w.count==0&&w.sunrise[sunDay(w,now)]!=0);
 data.clear();saved=w;assert(!parseWeather(data,w,err,now,100));assert(w.received==saved.received);
 std::ifstream live("tests/weather_live_2026-11-08.json");JsonDocument ld;assert(!deserializeJson(ld,live));time_t captured=1794142128;assert(parseWeather(ld,w,err,captured,100));assert(w.count==72&&nextHour(w,captured)>=0&&w.sunrise[sunDay(w,captured)]!=0);

 // Independent USNO primary phases, UTC, rounded to nearest minute by source.
 const char* full[]={"2026-01-03T10:03","2026-02-01T22:09","2026-03-03T11:38","2026-04-02T02:12","2026-05-01T17:23","2026-05-31T08:45","2026-06-29T23:56","2026-07-29T14:36","2026-08-28T04:18","2026-09-26T16:49","2026-10-26T04:12","2026-11-24T14:53","2026-12-24T01:28"};long maxError=0;
 for(auto s:full){tm t={};strptime(s,"%Y-%m-%dT%H:%M",&t);time_t expected=timegm(&t);Moon m=calculateMoon(expected-5*86400);assert(m.valid&&m.lit>=0&&m.lit<=100);long error=labs(m.full-expected);maxError=std::max(maxError,error);assert(error<180);}
 std::cout<<"PASS: 1890 schema-3 migrations, schema 1/2, 1023 ten-view masks, 10 selections, five dates, credentials, invalid config, DST, partial weather/sun, captured live JSON, 13 USNO full moons. Max UTC error = "<<maxError<<" seconds (2026 samples only).\n";
}
