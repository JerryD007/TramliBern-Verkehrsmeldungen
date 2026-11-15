#pragma once
#include "Model.h"
struct Quote {double value=0;time_t received=0;char date[11]="";char error[80]="Noch nicht geladen";bool valid=false;};
struct FinanceData {Quote btc,fx[3];};
inline bool financeConfigEqual(const Config&a,const Config&b){return !strcmp(a.btcCurrency,b.btcCurrency)&&!memcmp(a.fxBase,b.fxBase,sizeof(a.fxBase))&&!memcmp(a.fxQuote,b.fxQuote,sizeof(a.fxQuote));}
inline bool quoteStale(const Quote&q,time_t now,bool bitcoin){time_t date=0;bool oldDate=!bitcoin&&q.date[0]&&(!parseDate(q.date,date)||calendarDays(now,date)>7);return oldDate||!q.valid||now<q.received||now-q.received>(bitcoin?600:86400)||q.error[0];}
