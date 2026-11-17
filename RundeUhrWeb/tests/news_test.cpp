#include <cassert>
#include <fstream>
#include <iterator>
#include <cstdio>
#include "../RundeUhr/NewsFeed.h"
int main(int argc,char**argv){
 const char*xml="<?xml version='1.0'?><rss><channel><item><title>Test &amp; Welt &#252;</title><link>https://www.tagesschau.de/test.html</link><dc:date>2026-11-17T03:00:00Z</dc:date><content:encoded><![CDATA[<p>ignored</p>]]></content:encoded></item></channel></rss>";
 for(int chunk:{1,2,3,7,512}){NewsFeed f;for(size_t i=0;i<strlen(xml);i+=chunk)assert(f.write((const uint8_t*)xml+i,std::min(size_t(chunk),strlen(xml)-i))==std::min(size_t(chunk),strlen(xml)-i));NewsData n;assert(f.finish(n,1794890000));assert(n.count==1&&strstr(n.items[0].title,"Test & Welt"));}
 for(const char*s:{"<rss><channel>","<rss><channel></rss>","<!DOCTYPE rss><rss></rss>","<rss><channel><item><title>x</title></item></channel></rss>"}){NewsFeed f;f.write((const uint8_t*)s,strlen(s));NewsData n;n.count=2;assert(!f.finish(n,1794890000)&&n.count==2);}
 if(argc>1){std::ifstream in(argv[1]);std::string s((std::istreambuf_iterator<char>(in)),{});NewsFeed f;f.write((const uint8_t*)s.data(),s.size());NewsData n;assert(f.finish(n,time(nullptr)));assert(n.count);printf("PASS live RSS schema: %u bounded headlines, %zu bytes streamed\n",n.count,s.size());}
 puts("PASS RSS chunks/entities/CDATA/DTD rejection/truncation/cache preservation");
}
