#pragma once
#include "FinanceParse.h"
#include "FinanceTrust.h"
#include "TlsBufferSizes.h"
struct FinanceResult {Config config;FinanceData data;};
QueueHandle_t financeResultQueue;
inline void fetchQuote(Quote&q,const char*base,const char*currency,bool btc){
 if(!btc&&!strcmp(base,currency)){setQuoteValue(q,1);q.valid=true;q.received=time(nullptr);q.date[0]=0;q.error[0]=0;return;}
 WiFiClientSecure client;client.setCACert(FINANCE_TRUST);client.setHandshakeTimeout(8);
 HTTPClient http;http.setConnectTimeout(5000);http.setTimeout(6000);http.useHTTP10(true);
 String url=btc?String("https://api.coinbase.com/v2/prices/BTC-")+currency+"/spot":String("https://api.frankfurter.dev/v2/rate/")+base+"/"+currency;
 if(!http.begin(client,url)){strcpy(q.error,"HTTPS Start fehlgeschlagen");return;}
 http.addHeader("Accept","application/json");http.addHeader("Accept-Encoding","identity");http.setUserAgent("RundeUhr/3.1.0-rc2");
 int code=http.GET();q.http=code;
 if(code!=200){char detail[80]={};q.tls=client.lastError(detail,sizeof(detail));snprintf(q.error,sizeof(q.error),"HTTP %d / TLS %d",code,q.tls);http.end();return;}
 int length=http.getSize();if(length>4096){strcpy(q.error,"Antwort zu gross");http.end();return;}
 BoundedBody body(length>0?size_t(length):4096);
 if(!body.ready()){strcpy(q.error,"RAM fehlt");http.end();return;}
 int count=http.writeToStream(&body);http.end();client.stop();
 if(count<0||body.overflowed()||body.allocationFailed()||(length>=0&&count!=length)){strcpy(q.error,"Antwort unvollstaendig");return;}
 JsonDocument d;if(deserializeJson(d,body)||!parseFinance(d,q,base,currency,btc,time(nullptr)))strcpy(q.error,"Ungueltige Preisdaten");
}
// Match the compiled mbedTLS buffers, including TLS framing and cipher overhead.
// Keep the total-heap safety floor; success is admission, not a handshake guarantee.
inline bool financeMemoryReady(Quote&q){
 q.heapBefore=ESP.getFreeHeap();q.largestBefore=heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
 q.probeIn=RUNDE_TLS_INPUT_BYTES;q.probeOut=RUNDE_TLS_OUTPUT_BYTES;
 q.largestAfterFirst=0;q.memoryGate=1;
 if(q.heapBefore<65000)return false;
 void*a=malloc(q.probeIn);if(!a){q.memoryGate=2;return false;}
 q.largestAfterFirst=heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
 void*b=malloc(q.probeOut);q.memoryGate=b?0:3;
 bool ready=b!=nullptr;free(b);free(a);return ready;
}
// Called by the existing single network worker: never concurrent with weather TLS.
inline void financeTick(const Config&c,bool reset){
 static Config previous;static FinanceData data;static bool started=false;static uint32_t last[4]={};static bool attempted[4]={};static uint8_t cursor=0;
 if(!started||!financeConfigEqual(c,previous)||reset){data=FinanceData();memset(attempted,0,sizeof(attempted));previous=c;started=true;}
 uint32_t ms=millis();
 for(int j=0;j<4;j++){int i=(cursor+j)%4;bool enabled=i==0?(c.view==10||(c.mask&(1<<10))):(c.view==11||(c.mask&(1<<11)));if(!enabled)continue;
 Quote&q=i==0?data.btc:data.fx[i-1];uint32_t interval=q.memoryDeferred?15000UL:q.error[0]?300000UL:(i==0?120000UL:21600000UL);
 if(attempted[i]&&uint32_t(ms-last[i])<interval)continue;
 networkBusy=true;if(otaActive){networkBusy=false;return;}
 // Skip TLS instead of starving the web server when memory is already scarce.
 q.memoryDeferred=false;q.http=q.tls=0;
 bool identity=i>0&&!strcmp(c.fxBase[i-1],c.fxQuote[i-1]);
 if(!identity&&!financeMemoryReady(q)){q.memoryDeferred=true;snprintf(q.error,sizeof(q.error),"RAM %s: %lu B; Block %lu/%lu; in 15 s",q.memoryGate==1?"gesamt":q.memoryGate==2?"TLS Eingang":"TLS Ausgang",(unsigned long)q.heapBefore,(unsigned long)q.largestBefore,(unsigned long)q.largestAfterFirst);}
 else fetchQuote(q,i==0?"BTC":c.fxBase[i-1],i==0?c.btcCurrency:c.fxQuote[i-1],i==0);
 networkBusy=false;last[i]=millis();attempted[i]=true;cursor=(i+1)%4;
 FinanceResult result;result.config=c;result.data=data;xQueueOverwrite(financeResultQueue,&result);return;
 }
}
