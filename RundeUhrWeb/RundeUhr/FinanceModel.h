#pragma once
#include "Model.h"
struct Quote {int8_t trend=0;double value=0;time_t received=0;char date[11]="";char error[80]="Noch nicht geladen";bool valid=false;bool memoryDeferred=false;uint32_t heapBefore=0,largestBefore=0,largestAfterFirst=0;uint16_t probeIn=0,probeOut=0;uint8_t memoryGate=0;int http=0,tls=0;};
struct FinanceData {Quote btc,fx[3];};
inline bool financeConfigEqual(const Config&a,const Config&b){return !strcmp(a.btcCurrency,b.btcCurrency)&&!memcmp(a.fxBase,b.fxBase,sizeof(a.fxBase))&&!memcmp(a.fxQuote,b.fxQuote,sizeof(a.fxQuote));}
inline bool quoteStale(const Quote&q,time_t now,bool bitcoin){time_t date=0;bool oldDate=!bitcoin&&q.date[0]&&(!parseDate(q.date,date)||calendarDays(now,date)>7);return oldDate||!q.valid||now<q.received||now-q.received>(bitcoin?600:86400)||q.error[0];}

// Compare only successful observations of the same configured instrument.
inline void setQuoteValue(Quote&q,double value){q.trend=q.valid?(value>q.value?1:value<q.value?-1:0):0;q.value=value;}
