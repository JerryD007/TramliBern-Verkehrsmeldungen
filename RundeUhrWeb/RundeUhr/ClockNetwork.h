#pragma once
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <esp_heap_caps.h>
#include "Trust.h"
#include "BoundedBody.h"
#include "ParseData.h"
struct NetDiagnostics {bool busy=false;time_t attempt=0;int http=0,tls=0;uint32_t bytes=0,duration=0,largestBlock=0,heapBeforeTLS=0,networkStackFree=0;uint8_t trustProfile=0;};
struct NetResult {Weather weather;char error[192]="";uint32_t generation=0;NetDiagnostics diagnostics;};
struct NetConfig {Config config;uint32_t generation=0,refresh=0;};
QueueHandle_t netConfigQueue,netResultQueue;
inline bool requestJSON(String url,JsonDocument &doc,String &error,NetDiagnostics &diag,const char* rootPEM){
 WiFiClientSecure client;
 // Standard X.509 chain validation rather than the bundle verification callback.
 // Only two roots are parsed per connection to keep memory bounded.
 client.setCACert(rootPEM);client.setHandshakeTimeout(10);
 diag.heapBeforeTLS=ESP.getFreeHeap();diag.largestBlock=heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
 HTTPClient http;http.setConnectTimeout(7000);http.setTimeout(10000);http.useHTTP10(true);
 if(!http.begin(client,url)){error="HTTPS Start fehlgeschlagen";return false;}
 http.addHeader("Accept","application/json");http.addHeader("Accept-Encoding","identity");
 http.setUserAgent("RundeUhr/2.3.3 ESP32-C3");
 int code=http.GET();diag.http=code;
 if(code!=200){
  char tls[100]={};diag.tls=client.lastError(tls,sizeof(tls));
  error=code>0?"Wetter HTTP "+String(code):"HTTPS-Verbindung fehlgeschlagen";
  if(diag.tls)error+="; TLS "+String(diag.tls)+" "+String(tls);
  http.end();return false;
 }
 constexpr size_t maxBody=32768;int expected=http.getSize();
 if(expected>int(maxBody)){error="Antwort > 32 KiB";http.end();return false;}
 BoundedBody body(expected>0?size_t(expected):maxBody);
 if(!body.ready()){error="RAM fehlt vor Wetterempfang";http.end();return false;}
 int received=http.writeToStream(&body);http.end();client.stop();diag.bytes=body.size();
 if(body.overflowed()){error="Wetterantwort zu gross";return false;}
 if(body.allocationFailed()){error="RAM fehlt beim Wetterempfang";return false;}
 if(received<0){error="HTTP Lesen: "+HTTPClient::errorToString(received);return false;}
 if(expected>=0&&body.size()!=size_t(expected)){error="Antwort unvollstaendig";return false;}
 if(!body.size()){error="API Antwort leer";return false;}
 // Only parse fields used by the display. TLS buffers have been released above.
 JsonDocument filter;for(auto k:{"time","temperature_2m","weather_code","precipitation_probability","precipitation"})filter["hourly"][k]=true;
 for(auto k:{"time","sunrise","sunset"})filter["daily"][k]=true;filter["error"]=true;filter["reason"]=true;
 auto e=deserializeJson(doc,body.bytes(),body.size(),DeserializationOption::Filter(filter),DeserializationOption::NestingLimit(12));
 if(e){error="JSON: "+String(e.c_str())+" ("+String(body.size())+" B)";return false;}
 if(doc["error"]==true){error="API: "+String(doc["reason"]|"Unbekannter Fehler");return false;}
 return true;
}
inline bool fetchWeather(const Config &c,Weather &w,String &error,NetDiagnostics &diag){
 JsonDocument d;String url="https://api.open-meteo.com/v1/forecast?latitude="+String(c.lat,5)+"&longitude="+String(c.lon,5)+"&hourly=temperature_2m,weather_code,precipitation_probability,precipitation&daily=sunrise,sunset&timezone=Europe%2FZurich&timeformat=unixtime&forecast_days=3";
 static uint8_t preferredTrust=0;
 for(int attempt=0;attempt<2;attempt++){
  uint8_t profile=(preferredTrust+attempt)%2;diag.trustProfile=profile;
  diag.http=diag.tls=0;diag.bytes=0;error="";d.clear();
  if(requestJSON(url,d,error,diag,profile?TRUST_LEGACY:TRUST_CURRENT)){
   preferredTrust=profile;return parseWeather(d,w,error,time(nullptr),millis());
  }
  // Retry only certificate failures, never HTTP/rate-limit/JSON/RAM failures.
  if(diag.tls!=-12288&&diag.tls!=-9984)break;
 }
 return false;
}
#include "FinanceNetwork.h"
#include "AircraftNetwork.h"
#include "IssNetwork.h"
#include "QuakeNetwork.h"
#include "LaunchNetwork.h"
inline void networkTask(void*){
 NetConfig state={};bool configured=false,first=true,previousOnline=false;uint32_t last=0;bool success=false;
 for(;;){NetConfig next;if(xQueueReceive(netConfigQueue,&next,0)==pdTRUE){if(!configured||next.generation!=state.generation||next.refresh!=state.refresh)first=true;state=next;configured=true;}
  bool online=WiFi.status()==WL_CONNECTED;if(online&&!previousOnline)first=true;previousOnline=online;
  if(configured&&online&&!otaActive&&timeValid(time(nullptr))){
   uint32_t now=millis();
   // Preload all three products after NTP, independent of selected screen/cycle.
   if(first||uint32_t(now-last)>=(success?900000UL:60000UL)){
    networkBusy=true;
    if(otaActive){networkBusy=false;vTaskDelay(pdMS_TO_TICKS(150));continue;}
    first=false;last=now;NetResult r;r.generation=state.generation;r.diagnostics.busy=true;r.diagnostics.attempt=time(nullptr);
    r.diagnostics.largestBlock=heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
    strlcpy(r.error,"Wetter wird geladen ...",sizeof(r.error));xQueueOverwrite(netResultQueue,&r);
    String error;success=fetchWeather(state.config,r.weather,error,r.diagnostics);networkBusy=false;
    r.diagnostics.networkStackFree=uxTaskGetStackHighWaterMark(nullptr);r.diagnostics.busy=false;r.diagnostics.duration=millis()-now;strlcpy(r.error,error.c_str(),sizeof(r.error));xQueueOverwrite(netResultQueue,&r);
   }
  }
  if(configured&&WiFi.status()==WL_CONNECTED&&!otaActive&&timeValid(time(nullptr))){financeTick(state.config,false);aircraftTick(state.config);issTick(state.config);quakeTick(state.config);launchTick(state.config);}
  vTaskDelay(pdMS_TO_TICKS(150));
 }
}
