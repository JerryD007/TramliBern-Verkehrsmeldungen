#pragma once
#include "LaunchModel.h"
#include "NewsText.h"
static const unsigned NEWS_ADVANCE=24,NEWS_SPEED=90,NEWS_BAND=184;
struct NewsItem {char title[256]="",link[192]="";time_t at=0;};
struct NewsData {bool valid=false;uint8_t count=0,source=0;time_t received=0;int http=0;char error[80]="";NewsItem items[3];};
inline bool newsStale(const NewsData&n,time_t now){return !n.valid||now<n.received||now-n.received>10800;}

inline uint32_t newsHeadlineDuration(const char* title){unsigned width=newsDisplayText(title,nullptr,0)*NEWS_ADVANCE;return 3000UL+(width>NEWS_BAND?(width-NEWS_BAND)*1000UL/NEWS_SPEED:0);}
inline uint32_t newsCycleDuration(const Config&c,const NewsData&n,time_t now){for(int i=0;i<n.count;i++)if(n.items[i].at<=now&&now-n.items[i].at<=c.newsWindow*60)return newsHeadlineDuration(n.items[i].title);return 5000UL;}

struct NewsSession {
 NewsData selected;uint8_t next=0,source=255;uint32_t duration=5000;
 void begin(const Config&c,const NewsData&n,time_t now){
  selected=NewsData();selected.source=n.source;selected.valid=n.valid;selected.received=n.received;
  if(source!=n.source){next=0;source=n.source;}
  int eligible[3],count=0;for(int i=0;i<n.count;i++)if(n.items[i].at<=now&&now-n.items[i].at<=c.newsWindow*60)eligible[count++]=i;
  if(count){int i=eligible[next%count];next=(next+1)%count;selected.items[0]=n.items[i];selected.count=1;duration=newsHeadlineDuration(n.items[i].title);}else duration=5000;
 }
};
