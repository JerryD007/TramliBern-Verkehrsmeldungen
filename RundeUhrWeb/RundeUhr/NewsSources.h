#pragma once
struct NewsSource {const char*name;const char*url;const char*credit;};
constexpr NewsSource NEWS_SOURCES[]={
 {"tagesschau · Alle Meldungen","https://www.tagesschau.de/infoservices/alle-meldungen-100~rss2.xml","tagesschau.de"},
 {"tagesschau · Deutschland","https://www.tagesschau.de/inland/index~rss2.xml","tagesschau.de"},
 {"tagesschau · Ausland","https://www.tagesschau.de/ausland/index~rss2.xml","tagesschau.de"},
 {"tagesschau · Bayern","https://www.tagesschau.de/inland/regional/bayern/index~rss2.xml","tagesschau.de"},
 {"tagesschau · Berlin","https://www.tagesschau.de/inland/regional/berlin/index~rss2.xml","tagesschau.de"},
 {"SRF · Schweiz","https://www.srf.ch/news/bnf/rss/1890","srf.ch"},
 {"SRF · International","https://www.srf.ch/news/bnf/rss/1922","srf.ch"},
 {"ČT24 · Česko a svět","https://ct24.ceskatelevize.cz/rss/tema/vyber-redakce-84313","CT24"},
 {"CNN International · via Google News (EN)","https://news.google.com/rss/search?q=site%3Acnn.com%20when%3A1d&hl=en-US&gl=US&ceid=US%3Aen","CNN / Google"}
};
constexpr unsigned NEWS_SOURCE_COUNT=sizeof(NEWS_SOURCES)/sizeof(NEWS_SOURCES[0]);
