#pragma once
#include "LaunchModel.h"
#include "QuakeModel.h"
#include "NewsModel.h"
// Cycle selection is independent from urgent interruptions.
inline bool launchRelevant(const Config&c,const LaunchEvent&e,time_t now){return (!strcmp(e.status,"Go")||!strcmp(e.status,"TBC")||!strcmp(e.status,"TBD")||!strcmp(e.status,"Hold"))&&e.net>now&&e.net-now<=c.launchWindow*60;}
inline bool launchUrgent(const LaunchEvent&e,time_t now){return !strcmp(e.status,"Go")&&!strcmp(e.precision,"MIN")&&e.net>now&&e.net-now<=3600;}
inline bool quakeRelevant(const Config&c,const QuakeEvent&e,time_t now){
 if(e.at>now||now-e.at>c.quakeWindow*60||e.magnitude<c.quakeMagnitude)return false;
 if(!c.quakeRadius)return true;if(!e.location)return false;
 double a=(e.lat-c.lat)*M_PI/180,b=(e.lon-c.lon)*M_PI/180;double h=sin(a/2)*sin(a/2)+cos(c.lat*M_PI/180)*cos(e.lat*M_PI/180)*sin(b/2)*sin(b/2);
 return 6371*2*asin(sqrt(fmin(1.0,fmax(0.0,h))))<=c.quakeRadius;
}
inline bool quakeUrgent(const QuakeEvent&e,time_t now){return e.at<=now&&now-e.at<=3600&&e.magnitude>=6.0;}
inline uint32_t eventCycleMask(const Config&c,const LaunchData&l,const QuakeData&q,const NewsData&n,time_t now,bool online){
 uint32_t mask=c.mask;bool launch=false,quake=false,news=false;
 if(online&&timeValid(now)){
 if(!launchStale(l,now)&&!l.error[0])for(int k=0;k<l.count;k++)launch|=launchRelevant(c,l.events[k],now);
 if(!quakeStale(q,now)&&!q.error[0])for(int k=0;k<q.count;k++)quake|=quakeRelevant(c,q.events[k],now);
 if(!newsStale(n,now)&&!n.error[0])for(int k=0;k<n.count;k++)news|=n.items[k].at<=now&&now-n.items[k].at<=c.newsWindow*60;
 }if(!launch)mask&=~(1UL<<15);if(!quake)mask&=~(1UL<<14);if(!news)mask&=~(1UL<<16);return mask;
}
inline uint64_t eventHash(const char*s,uint64_t seed){for(;*s;s++)seed=(seed^uint8_t(*s))*1099511628211ULL;return seed?seed:1;}
inline uint64_t launchKey(const LaunchEvent&e){return eventHash(e.name,uint64_t(e.net)^1469598103934665603ULL);}
inline uint64_t quakeKey(const QuakeEvent&e){char b[96];snprintf(b,sizeof(b),"%lld/%.4f/%.4f",(long long)e.at,e.lat,e.lon);return eventHash(b,1469598103934665603ULL);}
// Manual selection always wins. Unexpired identities are never evicted.
struct EventPriority {
 struct Seen {uint64_t key=0;time_t expires=0;};Seen seen[64];uint64_t activeKey=0;bool historyDirty=false;int active=-1;uint32_t until=0,last=0;bool shown=false;
 bool known(uint64_t key){for(auto x:seen)if(x.key==key)return true;return false;}
 bool remember(uint64_t key,time_t expires,time_t now){for(auto &x:seen)if(!x.key||x.expires<now){x.key=key;x.expires=expires;historyDirty=true;return true;}return false;}
 int tick(const Config&c,const LaunchData&l,const QuakeData&q,const NewsData&n,time_t now,uint32_t ms,bool online){
 uint32_t mask=eventCycleMask(c,l,q,n,now,online)&c.mask;
 if(!c.automatic||!online){active=-1;return -1;}
 if(active>=0){if((mask&(1UL<<active))&&int32_t(until-ms)>0&&(active==14?c.quakePriority:c.launchPriority))return active;active=-1;}
 if(shown&&uint32_t(ms-last)<120000)return -1;
 uint64_t key=0;time_t expires=0;int candidate=-1;
 if(c.quakePriority&&(mask&(1UL<<14)))for(int i=0;i<q.count;i++)if(quakeUrgent(q.events[i],now)){uint64_t k=quakeKey(q.events[i]);if(!known(k)){key=k;expires=q.events[i].at+3600;candidate=14;break;}}
 if(candidate<0&&c.launchPriority&&(mask&(1UL<<15)))for(int i=0;i<l.count;i++)if(launchUrgent(l.events[i],now)){uint64_t k=launchKey(l.events[i]);if(!known(k)){key=k;expires=l.events[i].net;candidate=15;break;}}
 if(candidate>=0&&remember(key,expires,now)){activeKey=key;active=candidate;until=ms+20000;last=ms;shown=true;}return active;
 }
};
inline LaunchData filteredLaunch(const Config&c,const LaunchData&d,time_t now){LaunchData n=d;n.count=0;for(int i=0;i<d.count;i++)if(launchRelevant(c,d.events[i],now))n.events[n.count++]=d.events[i];return n;}
inline QuakeData filteredQuake(const Config&c,const QuakeData&d,time_t now){QuakeData n=d;n.count=0;for(int i=0;i<d.count;i++)if(quakeRelevant(c,d.events[i],now))n.events[n.count++]=d.events[i];return n;}

inline LaunchData priorityLaunch(const LaunchData&d,time_t now,uint64_t key){LaunchData n=d;n.count=0;for(int i=0;i<d.count;i++)if(launchUrgent(d.events[i],now)&&launchKey(d.events[i])==key)n.events[n.count++]=d.events[i];return n;}
inline QuakeData priorityQuake(const QuakeData&d,time_t now,uint64_t key){QuakeData n=d;n.count=0;for(int i=0;i<d.count;i++)if(quakeUrgent(d.events[i],now)&&quakeKey(d.events[i])==key)n.events[n.count++]=d.events[i];return n;}
