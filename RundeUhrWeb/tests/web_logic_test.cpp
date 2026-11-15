#include "../RundeUhr/ConfigCodec.h"
#include <cassert>
#include <iostream>
time_t at(const char* date,int h=0,int m=0,int sec=0){time_t t;assert(parseDate(date,t));tm local;localtime_r(&t,&local);local.tm_hour=h;local.tm_min=m;local.tm_sec=sec;local.tm_isdst=-1;return mktime(&local);}
void fixture(JsonDocument& d){deserializeJson(d,R"({"schema":4,"city":"Bern","lat":46.948,"lon":7.4474,"event":"Ferien","date":"","view":7,"mask":129,"interval":15,"automatic":true,"countdownType":2,"animations":true,"countdownDates":["2027-01-01","1990-01-01","","",""],"customEvent":"Mein Ereignis","ssid":"SavedWiFi","pass":"test_password"})");d["countdownDates"][1]="2024-01-01";}
int main(){setenv("TZ",ZURICH_TZ,1);tzset();Config c;
assert(!weekendPictureOnly(at("2026-11-13",15,59,59),c));assert(weekendPictureOnly(at("2026-11-13",16),c));assert(weekendPictureOnly(at("2026-11-15",23,59,59),c));assert(!weekendPictureOnly(at("2026-11-16"),c));
time_t target;assert(countdownTarget(c,2,at("2026-11-13",15),target));assert(target==at("2026-11-13",16));assert(countdownTarget(c,2,at("2026-11-16"),target));assert(target==at("2026-11-20",16));
// DST boundaries use the local calendar, not a fixed number of seconds.
for(const char* day:{"2026-03-29","2026-10-25"}){assert(weekendPictureOnly(at(day,3),c));assert(countdownTarget(c,2,at(day,3),target));tm t;localtime_r(&target,&t);assert(t.tm_wday==5&&t.tm_hour==16);}
c.weekendStartDay=6;c.weekendStartMinute=600;c.weekendEndDay=0;c.weekendEndMinute=1200;assert(!weekendPictureOnly(at("2026-11-14",9,59),c));assert(weekendPictureOnly(at("2026-11-14",10),c));assert(!weekendPictureOnly(at("2026-11-15",20),c));
JsonDocument d;fixture(d);bool migrated=false;Config restored;assert(loadConfig(d,restored,migrated));assert(!strcmp(restored.ssid,"SavedWiFi"));assert(!strcmp(restored.pass,"test_password"));assert(restored.view==7&&restored.mask==129&&restored.automatic);assert(restored.weekendStartMinute==960);d["weekendStartMinute"]=900;assert(decodeConfig(d,restored));assert(restored.weekendStartMinute==900);d["weekendStartMinute"]=1440;assert(!decodeConfig(d,restored));assert(restored.weekendStartMinute==900);d["weekendStartMinute"]=0;d["weekendStartDay"]=1;d["weekendEndDay"]=1;d["weekendEndMinute"]=0;assert(!decodeConfig(d,restored));fixture(d);d["mask"]=0;assert(!decodeConfig(d,restored));assert(nextView(0,129)==7);assert(nextView(7,129)==0);
fixture(d);strcpy(restored.countdownDates[1],"2024-02-29");assert(countdownTarget(restored,1,at("2027-02-28",16),target));assert(target==at("2027-02-28"));assert(countdownTarget(restored,1,at("2027-03-01"),target));assert(target==at("2028-02-29"));
std::cout<<"PASS: Friday 16:00, Sunday/Monday boundary, custom weekend, Zurich DST, schema-4 migration, credentials preserved, invalid settings atomic rejection, cycle mask, birthday leap-year\n";
}
