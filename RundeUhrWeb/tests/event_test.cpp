#include <cassert>
#include "../RundeUhr/EventPolicy.h"
#include "../RundeUhr/ConfigCodec.h"
int main(){time_t now=1794890000;Config c;c.automatic=true;c.mask=(1UL<<14)|(1UL<<15)|(1UL<<16)|1;LaunchData l;QuakeData q;NewsData n;assert(eventCycleMask(c,l,q,n,now,true)==1);l.valid=true;l.received=now;l.count=1;auto&e=l.events[0];e.net=now+3599;strcpy(e.status,"Go");strcpy(e.precision,"MIN");assert(eventCycleMask(c,l,q,n,now,true)&(1UL<<15));e.net=now-1;assert(!(eventCycleMask(c,l,q,n,now,true)&(1UL<<15)));e.net=now+60;
 q.valid=true;q.generated=now;q.count=1;q.events[0].at=now-10;q.events[0].magnitude=5;assert(eventCycleMask(c,l,q,n,now,true)&(1UL<<14));c.quakeRadius=100;assert(!(eventCycleMask(c,l,q,n,now,true)&(1UL<<14)));c.quakeRadius=0;
 EventPriority p;assert(p.tick(c,l,q,n,now,1000,true)==14);assert(p.tick(c,l,q,n,now,21000,true)==-1);assert(p.tick(c,l,q,n,now,122000,true)==15);assert(p.tick(c,l,q,n,now,243000,true)==-1);assert(eventCycleMask(c,l,q,n,now,false)==1);
 n.valid=true;n.received=now;n.count=1;n.items[0].at=now;assert(eventCycleMask(c,l,q,n,now,true)&65536);assert(mergedScreenMask(65537)==65537);assert(nextView(0,65537)==16);c.automatic=false;EventPriority p2;assert(p2.tick(c,l,q,n,now,100,true)==-1);
 n.count=2;n.items[0].at=now-20000;n.items[1].at=now;strcpy(n.items[0].title,"old");strcpy(n.items[1].title,"longer current headline");assert(newsCycleDuration(c,n,now)==newsHeadlineDuration(n.items[1].title));assert(newsHeadlineDuration("abc")==3000);
 puts("PASS skip/priority/freshness/area/magnitude/manual/cooldown/32-bit mask");}
