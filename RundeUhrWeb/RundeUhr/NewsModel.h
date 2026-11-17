#pragma once
#include "LaunchModel.h"
struct NewsItem {char title[256]="",link[192]="";time_t at=0;};
struct NewsData {bool valid=false;uint8_t count=0,source=0;time_t received=0;int http=0;char error[80]="";NewsItem items[3];};
inline bool newsStale(const NewsData&n,time_t now){return !n.valid||now<n.received||now-n.received>10800;}

inline uint32_t newsHeadlineDuration(const char* title){return 5000UL+uint32_t(strlen(title))*600UL;}
inline uint32_t newsCycleDuration(const Config&c,const NewsData&n,time_t now){for(int i=0;i<n.count;i++)if(n.items[i].at<=now&&now-n.items[i].at<=c.newsWindow*60)return newsHeadlineDuration(n.items[i].title);return 5000UL;}
