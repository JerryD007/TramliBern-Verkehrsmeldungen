#pragma once
#include <math.h>
#include <time.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "Currencies.h"
constexpr int VIEW_COUNT=12;
constexpr const char* ZURICH_TZ="CET-1CEST,M3.5.0,M10.5.0/3";
struct Config {
 char ssid[33]="",pass[65]="",city[49]="Bern",event[49]="Ferien",date[11]="";
 char deviceName[49]="";
 char countdownDates[5][11]={}; char customEvent[49]="Mein Ereignis";
 uint8_t countdownType=0; bool animations=true;
 double lat=46.948,lon=7.4474;
 uint16_t mask=1023,interval=15;
 uint8_t view=0; bool automatic=false;
 char btcCurrency[4]="CHF";
 char fxBase[3][4]={"CHF","EUR","EUR"},fxQuote[3][4]={"CZK","CZK","CHF"};
 // Optional schema-4 extension. Old firmware ignores these fields.
 uint8_t weekendStartDay=5,weekendEndDay=1;
 uint16_t weekendStartMinute=960,weekendEndMinute=0;
};
constexpr const char* COUNTDOWN_NAMES[]={"Ferien","Geburtstag","Wochenende","Arschloch weg","Frei wählbar"};
inline void syncCountdown(Config &c){strcpy(c.event,c.countdownType==4?c.customEvent:COUNTDOWN_NAMES[c.countdownType]);strcpy(c.date,c.countdownDates[c.countdownType]);}
inline const char* countdownName(const Config&c,int type){return type==4?c.customEvent:COUNTDOWN_NAMES[type];}
struct Weather {
 bool valid=false; uint32_t fetched=0; time_t received=0;
 int count=0,days=0; time_t hours[72]={},sunrise[3]={},sunset[3]={},day[3]={};
 uint8_t fields[72]={}; // 1 temperature, 2 symbol, 4 probability, 8 amount
 float temps[72]={},amount[72]={}; int16_t codes[72]={},probability[72]={};
};
struct Moon {bool valid=false;double phase=0,lit=0;time_t full=0;};
inline bool timeValid(time_t t){return t>=1704067200LL&&t<4102444800LL;}
inline int nextView(int view,uint16_t mask){for(int i=1;i<=VIEW_COUNT;i++){int n=(view+i)%VIEW_COUNT;if(mask&(1<<n))return n;}return view;}
inline int oldView(int v){const int map[10]={0,-1,-1,1,2,3,-1,4,-1,5};return v>=0&&v<10?map[v]:-1;}
inline void migrateViews(int view,int mask,Config &c){int v=oldView(view);c.view=v<0?0:v;c.mask=0;for(int i=0;i<10;i++){int n=oldView(i);if(n>=0&&(mask&(1<<i)))c.mask|=1<<n;}if(!c.mask)c.mask=1;}
inline bool parseDate(const char* s,time_t &result){
 if(strlen(s)!=10||s[4]!='-'||s[7]!='-')return false;
 for(int i=0;i<10;i++)if(i!=4&&i!=7&&(s[i]<'0'||s[i]>'9'))return false;
 int y,m,d;if(sscanf(s,"%4d-%2d-%2d",&y,&m,&d)!=3||y<2024||y>2099||m<1||m>12||d<1||d>31)return false;
 tm t={};t.tm_year=y-1900;t.tm_mon=m-1;t.tm_mday=d;t.tm_isdst=-1;result=mktime(&t);
 return t.tm_year==y-1900&&t.tm_mon==m-1&&t.tm_mday==d;
}
inline long calendarDays(time_t target,time_t now){tm a,b;localtime_r(&target,&a);localtime_r(&now,&b);a.tm_hour=b.tm_hour=12;a.tm_min=b.tm_min=a.tm_sec=b.tm_sec=0;a.tm_isdst=b.tm_isdst=-1;return lround(difftime(mktime(&a),mktime(&b))/86400);}
inline int nextHour(const Weather&w,time_t now){for(int i=0;i<w.count;i++)if(w.hours[i]>now)return i;return -1;}
inline int sunDay(const Weather&w,time_t now){for(int i=0;i<w.days;i++)if(calendarDays(w.day[i],now)==0)return i;return -1;}
inline bool weatherStale(const Weather&w,time_t now){return w.valid&&(!timeValid(now)||now<w.received||now-w.received>1800);}

// Recurring dates are derived from local Zurich calendar time, never fixed seconds.
inline bool countdownTarget(const Config &c,int type,time_t now,time_t &target){
 if(type!=1&&type!=2)return parseDate(c.countdownDates[type],target);
 if(!timeValid(now))return false;
 tm today={};localtime_r(&now,&today);
 if(type==2){
  int days=(5-today.tm_wday+7)%7;
  days=(c.weekendStartDay-today.tm_wday+7)%7;
  if(days==0 && today.tm_hour*60+today.tm_min>=c.weekendStartMinute)days=7;
  today.tm_mday+=days;today.tm_hour=c.weekendStartMinute/60;today.tm_min=c.weekendStartMinute%60;today.tm_sec=0;today.tm_isdst=-1;
  target=mktime(&today);return true;
 }
 time_t stored;if(!parseDate(c.countdownDates[1],stored))return false;
 tm birthday={};localtime_r(&stored,&birthday);
 for(int year=today.tm_year;year<=today.tm_year+1;year++){
  tm next={};next.tm_year=year;next.tm_mon=birthday.tm_mon;next.tm_mday=birthday.tm_mday;next.tm_isdst=-1;
  // February 29 birthdays use February 28 in non-leap years.
  int y=year+1900;bool leap=(y%4==0&&(y%100!=0||y%400==0));
  if(next.tm_mon==1&&next.tm_mday==29&&!leap)next.tm_mday=28;
  target=mktime(&next);if(calendarDays(target,now)>=0)return true;
 }
 return false;
}

inline bool weekendPictureOnly(time_t now,const Config& c=Config()){
 if(!timeValid(now))return false;
 tm local={};localtime_r(&now,&local);
 const int week=7*1440,start=c.weekendStartDay*1440+c.weekendStartMinute,end=c.weekendEndDay*1440+c.weekendEndMinute;
 const int minute=local.tm_wday*1440+local.tm_hour*60+local.tm_min;
 return (minute-start+week)%week < (end-start+week)%week;
}
