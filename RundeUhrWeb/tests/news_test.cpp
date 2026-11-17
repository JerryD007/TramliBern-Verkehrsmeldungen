#include <cassert>
#include <fstream>
#include <iterator>
#include <cstdio>
#include "../RundeUhr/NewsFeed.h"
int main(int argc,char**argv){
 const char*xml="<?xml version='1.0'?><rss><channel><item><title>Test &amp; Welt &#252;</title><link>https://www.tagesschau.de/test.html</link><dc:date>2026-11-17T03:00:00Z</dc:date><content:encoded><![CDATA[<p>ignored</p>]]></content:encoded></item></channel></rss>";
 for(int chunk:{1,2,3,7,512}){NewsFeed f;for(size_t i=0;i<strlen(xml);i+=chunk)assert(f.write((const uint8_t*)xml+i,std::min(size_t(chunk),strlen(xml)-i))==std::min(size_t(chunk),strlen(xml)-i));NewsData n;assert(f.finish(n,1794890000));assert(n.count==1&&strstr(n.items[0].title,"Test & Welt"));}
 for(const char*s:{"<rss><channel>","<rss><channel></rss>","<!DOCTYPE rss><rss></rss>","<rss><channel><item><title>x</title></item></channel></rss>"}){NewsFeed f;f.write((const uint8_t*)s,strlen(s));NewsData n;n.count=2;assert(!f.finish(n,1794890000)&&n.count==2);}
 for(const char*url:{"https://ct24.ceskatelevize.cz/clanek/test","https://news.google.com/rss/articles/ABCD"}){bool cnn=strstr(url,"google");std::string x="<rss><channel><item><title>Češi &amp; svět</title><link>";x+=url;x+="</link><pubDate>Tue, 17 Nov 2026 18:00:00 GMT</pubDate><source url='https://www.cnn.com'>CNN</source></item></channel></rss>";NewsFeed f(cnn);f.write((const uint8_t*)x.data(),x.size());NewsData n;assert(f.finish(n,1794940000)&&n.count==1);assert(strstr(n.items[0].title,"Češi"));}
 time_t old;assert(newsUTC("2023-04-22T03:41:39Z",old));assert(!newsUTC("2023-02-29T00:00:00Z",old));
 std::string longXML="<rss><channel><item><title>Test</title><link>https://ct24.ceskatelevize.cz/clanek/test</link><pubDate>Tue, 17 Nov 2026 18:00:00 GMT</pubDate><media:content url='https://example.com/";longXML+=std::string(8000,'x');longXML+="'/></item></channel></rss>";NewsFeed longFeed;for(auto ch:longXML)longFeed.write(uint8_t(ch));NewsData longData;assert(longFeed.finish(longData,1794940000)&&longData.count==1);
 if(argc>1){std::ifstream in(argv[1]);std::string s((std::istreambuf_iterator<char>(in)),{});NewsFeed f(argc>2);for(size_t k=0;k<s.size();k++){f.write(uint8_t(s[k]));if(f.failed()){fprintf(stderr,"RSS failure at byte %zu, context: %.240s\n",k,s.c_str()+(k>120?k-120:0));break;}}NewsData n;assert(f.finish(n,time(nullptr)));assert(n.count);for(int i=0;i<n.count;i++)printf("NEWS_AT %lld\n",(long long)n.items[i].at);printf("PASS live RSS schema: %u bounded headlines, %zu bytes streamed\n",n.count,s.size());}
 puts("PASS RSS chunks/entities/CDATA/DTD rejection/truncation/cache preservation");
}
