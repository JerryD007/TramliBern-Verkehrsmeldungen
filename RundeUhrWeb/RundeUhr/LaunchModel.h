#pragma once
#include "Model.h"
struct LaunchEvent {char name[128]="",status[16]="",precision[8]="";time_t net=0;};
struct LaunchData {bool valid=false;uint8_t count=0;time_t received=0,nextAttempt=0;int http=0;char error[80]="";LaunchEvent events[3];};
inline bool launchStale(const LaunchData&d,time_t now){return !d.valid||now<d.received||now-d.received>7200;}
inline bool launchLeap(int y){return y%4==0&&(y%100!=0||y%400==0);}
// UTC parser independent of process/global timezone; never changes the clock's TZ.
inline bool launchUTC(const char*s,time_t&out){
 if(!s||strlen(s)!=20||s[4]!='-'||s[7]!='-'||s[10]!='T'||s[13]!=':'||s[16]!=':'||s[19]!='Z')return false;
 for(int i=0;i<19;i++)if(i!=4&&i!=7&&i!=10&&i!=13&&i!=16&&(s[i]<'0'||s[i]>'9'))return false;
 int y,m,d,h,n,sec;if(sscanf(s,"%4d-%2d-%2dT%2d:%2d:%2dZ",&y,&m,&d,&h,&n,&sec)!=6||y<2024||y>2099||m<1||m>12||h>23||n>59||sec>59)return false;
 int days[]={31,28+int(launchLeap(y)),31,30,31,30,31,31,30,31,30,31};if(d<1||d>days[m-1])return false;
 int64_t total=0;for(int z=2024;z<y;z++)total+=365+int(launchLeap(z));for(int z=0;z<m-1;z++)total+=days[z];
 out=time_t(1704067200LL+(total+d-1)*86400+h*3600+n*60+sec);return true;
}
inline uint32_t launchRetry(const char*s,time_t now){
 // Default one hour for 429. Support both Retry-After seconds and HTTP date.
 if(!s||!*s)return 3600000UL;char*end;unsigned long seconds=strtoul(s,&end,10);
 if(end!=s&&!*end){if(seconds<3600)seconds=3600;if(seconds>604800)seconds=604800;return uint32_t(seconds)*1000UL;}
 char weekday[4],month[4],zone[4];int day,year,h,m,sec;
 if(sscanf(s,"%3s, %d %3s %d %d:%d:%d %3s",weekday,&day,month,&year,&h,&m,&sec,zone)==8&&!strcmp(zone,"GMT")){
  const char*months[]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};int mon=0;for(int i=0;i<12;i++)if(!strcmp(month,months[i]))mon=i+1;
  char iso[24];snprintf(iso,sizeof(iso),"%04d-%02d-%02dT%02d:%02d:%02dZ",year,mon,day,h,m,sec);time_t at;
  if(launchUTC(iso,at)&&at>now){uint64_t delta=uint64_t(at-now);return uint32_t(delta<3600?3600:delta>604800?604800:delta)*1000UL;}
 }return 3600000UL;
}
inline const char* launchStatus(const LaunchEvent&e){
 if(!strcmp(e.status,"Go"))return "Geplant";if(!strcmp(e.status,"TBD"))return "Termin offen";if(!strcmp(e.status,"TBC"))return "Unbestaetigt";if(!strcmp(e.status,"Hold"))return "Unterbrochen";if(!strcmp(e.status,"In Flight"))return "Im Flug (Quelle)";if(!strcmp(e.status,"Success"))return "Erfolg (Quelle)";if(!strcmp(e.status,"Failure"))return "Fehler (Quelle)";return "Status offen";
}
