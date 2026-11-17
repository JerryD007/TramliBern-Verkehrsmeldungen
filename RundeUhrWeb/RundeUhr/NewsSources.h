#pragma once
struct NewsSource {const char*name;const char*url;const char*credit;};
constexpr NewsSource NEWS_SOURCES[]={
 {"tagesschau · Alle Meldungen","https://www.tagesschau.de/infoservices/alle-meldungen-100~rss2.xml","tagesschau.de"},
 {"tagesschau · Deutschland","https://www.tagesschau.de/inland/index~rss2.xml","tagesschau.de"},
 {"tagesschau · Ausland","https://www.tagesschau.de/ausland/index~rss2.xml","tagesschau.de"},
 {"tagesschau · Bayern","https://www.tagesschau.de/inland/regional/bayern/index~rss2.xml","tagesschau.de"},
 {"tagesschau · Baden-Wuerttemberg","https://www.tagesschau.de/inland/regional/baden-wuerttemberg/index~rss2.xml","tagesschau.de"},
 {"SRF · Schweiz","https://www.srf.ch/news/bnf/rss/1890","srf.ch"},
 {"SRF · International","https://www.srf.ch/news/bnf/rss/1922","srf.ch"}
};
constexpr unsigned NEWS_SOURCE_COUNT=sizeof(NEWS_SOURCES)/sizeof(NEWS_SOURCES[0]);
