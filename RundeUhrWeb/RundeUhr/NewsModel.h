#pragma once
#include "LaunchModel.h"
struct NewsItem {char title[256]="",link[192]="";time_t at=0;};
struct NewsData {bool valid=false;uint8_t count=0,source=0;time_t received=0;int http=0;char error[80]="";NewsItem items[3];};
inline bool newsStale(const NewsData&n,time_t now){return !n.valid||now<n.received||now-n.received>10800;}
