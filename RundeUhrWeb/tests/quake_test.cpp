#include "../RundeUhr/QuakeParse.h"
#include <cassert>
#include <iostream>
#include <fstream>
int main(int argc,char**argv){time_t now=1794850000;JsonDocument d;d["type"]="FeatureCollection";d["metadata"]["generated"]=int64_t(now)*1000;d["metadata"]["count"]=0;d["features"].to<JsonArray>();QuakeData q;assert(parseQuake(d,q,now)&&q.valid&&q.count==0);
for(int k=0;k<8;k++){auto f=d["features"].add<JsonObject>();f["properties"]["type"]="earthquake";f["properties"]["mag"]=4.5+k*.1;f["properties"]["time"]=int64_t(now-1000+k*100)*1000;f["properties"]["place"]="Test region";f["geometry"]["coordinates"][0]=1;f["geometry"]["coordinates"][1]=1;f["geometry"]["coordinates"][2]=10;}d["metadata"]["count"]=8;assert(parseQuake(d,q,now)&&q.count==5);assert(q.events[0].at==now-300&&q.events[4].at==now-700);assert(!quakeStale(q,now+900)&&quakeStale(q,now+901));
auto keep=q;d["features"][0]["properties"]["mag"]=nullptr;assert(!parseQuake(d,q,now));assert(q.events[0].at==keep.events[0].at);d["features"][0]["properties"]["mag"]=4.5;d["metadata"]["generated"]=int64_t(now-901)*1000;assert(!parseQuake(d,q,now));d["metadata"]["generated"]=int64_t(now+61)*1000;assert(!parseQuake(d,q,now));d["metadata"]["generated"]=int64_t(now)*1000;d["metadata"]["count"]=9;assert(!parseQuake(d,q,now));assert(nextView(13,(1<<13)|(1<<14))==14);assert(nextView(14,1<<13)==13);
// A successful feed can legitimately have zero matches under a 60-minute filter.
d["metadata"]["count"]=8;for(auto f:d["features"].as<JsonArray>())f["properties"]["time"]=int64_t(now-7200)*1000;
Config filter;assert(parseQuake(d,q,now,&filter)&&q.valid&&q.count==0);
filter.quakeWindow=1440;assert(parseQuake(d,q,now,&filter)&&q.count==5);
filter.quakeMagnitude=9;assert(parseQuake(d,q,now,&filter)&&q.valid&&q.count==0);
filter.quakeMagnitude=4.5;filter.quakeRadius=1;assert(parseQuake(d,q,now,&filter)&&q.count==0);
filter.quakeRadius=0;assert(parseQuake(d,q,now,&filter)&&q.count==5);
if(argc>1){std::ifstream stream(argv[1]);assert(!deserializeJson(d,stream));assert(parseQuake(d,q,time(nullptr)));std::cout<<"Live schema valid; events="<<int(q.count)<<"\n";}
std::cout<<"PASS quake empty, newest-five order, malformed fields, stale/future, atomic retention and cycle\n";
}
