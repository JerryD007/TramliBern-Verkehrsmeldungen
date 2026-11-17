#include <Arduino.h>
// 12 KiB task stack; monitor free high-water marks on the device.
#include <SPI.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>
#include <sys/time.h>
#include <lwip/sockets.h>
#include <errno.h>
#include "Model.h"
#include "DeviceName.h"
#include "AircraftRender.h"
#include "IssRender.h"
#include "QuakeRender.h"
#include "LaunchRender.h"
#include "NewsRender.h"
#include "EventPolicy.h"
#include "PaletteCanvas.h"
#include "Moon.h"
#include "WifiScan.h"
#include <esp_sntp.h>
#include "ConfigCodec.h"
#include <atomic>
#include <new>
#include <type_traits>
std::atomic<bool> otaActive{false}, networkBusy{false};
#include "ClockNetwork.h"
#include "WebApp.h"
SET_LOOP_TASK_STACK_SIZE(12288);
constexpr int TFT_SCL=4,TFT_SDA=6,TFT_DC=5,TFT_CS=7,TFT_RST=3;
Adafruit_GC9A01A display(TFT_CS,TFT_DC,TFT_RST);
PaletteCanvas canvas(240,240);
WebServer server(80);DNSServer dns;Preferences prefs;
Config cfg;Weather weather;Moon moon;FinanceData finance;AircraftData aircraft;IssData iss;QuakeData quake;LaunchData launches;NewsData news;EventPriority eventPriority;int eventView=-1;uint32_t manualUntil=0,renderSince=0;int renderedView=-1;
#include "ReceiveScratch.h"
int actualView(time_t now,bool online){if(manualUntil&&int32_t(manualUntil-millis())>0)return cfg.view;return eventView>=0?eventView:aircraftDisplayView(cfg,aircraft,now,online);}
volatile uint32_t ntpLast=0;
void onTimeSync(struct timeval *tv){ntpLast=tv->tv_sec;}
uint32_t lastMoon=0;
NetDiagnostics diagnostics;
String weatherError="Noch nicht abgerufen",apName,apKey,token;
bool apActive=false,dirty=false,wasConnected=false;
uint32_t dirtyAt=0,lastSwitch=0,lastDraw=0,offlineSince=0,lastRetry=0,apSince=0,generation=1,showSetupUntil=0,refreshRequest=0,lastManualRefresh=0;
bool firstDraw=true;
String hostName,deviceSuffix;uint32_t rebootAt=0;
void sendConfig(){NetConfig c={cfg,generation,refreshRequest};xQueueOverwrite(netConfigQueue,&c);}
void changed(){dirty=true;dirtyAt=millis();sendConfig();}
void configJSON(JsonDocument &d,bool privateData=false){d["schema"]=4;d["newsFeed"]=cfg.newsFeed;d["launchPriority"]=cfg.launchPriority;d["quakePriority"]=cfg.quakePriority;d["launchWindow"]=cfg.launchWindow;d["quakeWindow"]=cfg.quakeWindow;d["quakeMagnitude"]=cfg.quakeMagnitude;d["quakeRadius"]=cfg.quakeRadius;d["newsWindow"]=cfg.newsWindow;d["weekendTransition"]=cfg.weekendTransition;d["aircraftInRange"]=cfg.aircraftInRange;d["aircraftPriority"]=cfg.aircraftPriority;d["aircraftPlace"]=cfg.aircraftPlace;d["aircraftLat"]=cfg.aircraftLat;d["aircraftLon"]=cfg.aircraftLon;d["aircraftRadius"]=cfg.aircraftRadius;d["btcCurrency"]=cfg.btcCurrency;auto bases=d["fxBase"].to<JsonArray>();auto quotes=d["fxQuote"].to<JsonArray>();for(int i=0;i<3;i++){bases.add(cfg.fxBase[i]);quotes.add(cfg.fxQuote[i]);}d["weekendStartDay"]=cfg.weekendStartDay;d["weekendEndDay"]=cfg.weekendEndDay;d["weekendStartMinute"]=cfg.weekendStartMinute;d["weekendEndMinute"]=cfg.weekendEndMinute;d["deviceName"]=cfg.deviceName;d["countdownType"]=cfg.countdownType;d["animations"]=cfg.animations;d["customEvent"]=cfg.customEvent;JsonArray dates=d["countdownDates"].to<JsonArray>();for(int k=0;k<5;k++)dates.add(cfg.countdownDates[k]);d["city"]=cfg.city;d["lat"]=cfg.lat;d["lon"]=cfg.lon;d["event"]=cfg.event;d["date"]=cfg.date;d["view"]=cfg.view;d["automatic"]=cfg.automatic;d["mask"]=cfg.mask;d["interval"]=cfg.interval;d["ssid"]=cfg.ssid;if(privateData)d["pass"]=cfg.pass;}
void saveConfig(){JsonDocument d;configJSON(d,true);String s;serializeJson(d,s);if(prefs.getString("config","")!=s)prefs.putString("config",s);dirty=false;}
void startAP(){
 if(apActive)return;
 if(!WiFi.softAP(apName.c_str(),apKey.c_str())){Serial.println("RundeUhr: benanntes WLAN konnte nicht starten");return;}
 dns.start(53,"*",WiFi.softAPIP());apActive=true;apSince=millis();
}
void reply(int code,String message){JsonDocument d;d["ok"]=code==200;d["message"]=message;String s;serializeJson(d,s);server.send(code,"application/json",s);}
bool authorized(){if(server.header("X-Uhr-Token")!=token){reply(403,"Sitzung abgelaufen. Seite neu laden.");return false;}return true;}
bool body(JsonDocument &d){if(!authorized())return false;if(otaActive){reply(409,"Update laeuft firmware");return false;}if(server.arg("plain").length()>1800||deserializeJson(d,server.arg("plain"))){reply(400,"Ungueltige Eingabe");return false;}return true;}
bool frameWrite(NetworkClient &client,const uint8_t *data,size_t size,uint32_t started){
 while(size){if(uint32_t(millis()-started)>3000)return false;
  int n=::send(client.fd(),data,size,MSG_DONTWAIT);
  if(n>0){data+=n;size-=n;}else if(n<0&&(errno==EAGAIN||errno==EWOULDBLOCK)){delay(1);}else return false;
 }return true;
}
#include "MultiWifi.h"
#include "Boards.h"
#include "WebUpdate.h"
#include "FleetCors.h"
#include "SystemRoutes.h"
void routes(){
 const char* headers[]={"X-Uhr-Token","Origin","Access-Control-Request-Private-Network"};server.collectHeaders(headers,3);fleetRoutes();
 server.on("/",HTTP_GET,[]{server.sendHeader("Cache-Control","no-store");server.sendHeader("Content-Encoding","gzip");server.send_P(200,"text/html; charset=utf-8",(const char*)WEB_APP, sizeof(WEB_APP));});
 server.on("/api/state",HTTP_GET,[]{JsonDocument d;configJSON(d);time_t now=time(nullptr);d["firmware"]="3.2.0-rc24";d["imageSHA256"]=runningImageSHA();d["model"]=ESP.getChipModel();d["flash"]=ESP.getFlashChipSize();d["sketchSize"]=ESP.getSketchSize();d["uptime"]=millis()/1000;d["rssi"]=WiFi.RSSI();d["bootId"]=token;
  const char* viewNames[]={"Analoguhr","Wetter & Regen","Wetter & Regen","Sonne & Mond","Sonne & Mond","Ferien","Geburtstag","Wochenende","Arschloch weg","Frei wählbar","Bitcoin","Währungen","Nächstes Flugzeug","ISS Radar","Erdbeben","Raketenstarts","Nachrichten"};auto vn=d["screenNames"].to<JsonArray>();for(auto name:viewNames)vn.add(name);
 auto air=d["aircraft"].to<JsonObject>();d["displayView"]=actualView(now,WiFi.status()==WL_CONNECTED);d["eventPriorityView"]=eventView;d["effectiveMask"]=eventCycleMask(cfg,launches,quake,news,now,WiFi.status()==WL_CONNECTED);d["aircraftPriorityActive"]=aircraftPriorityActive(cfg,aircraft,now,WiFi.status()==WL_CONNECTED);air["origin"]=aircraft.origin;air["destination"]=aircraft.destination;air["routeHTTP"]=aircraft.routeHTTP;air["approaching"]=aircraft.approaching;air["checked"]=aircraft.checked;air["found"]=aircraft.found;air["callsign"]=aircraft.callsign;air["hex"]=aircraft.hex;air["registration"]=aircraft.registration;air["type"]=aircraft.type;air["kind"]=aircraft.kind==AircraftKind::Helicopter?"helicopter":aircraft.kind==AircraftKind::Plane?"plane":"unknown";air["distance"]=aircraft.distance;if(aircraft.altValid)air["altitude"]=aircraft.altitude;if(aircraft.speedValid)air["speed"]=aircraft.speed;air["received"]=int64_t(aircraft.received);air["positionAt"]=int64_t(aircraft.positionAt);air["error"]=aircraft.error;air["stale"]=aircraftStale(aircraft,now);air["http"]=aircraft.http;air["tls"]=aircraft.tls;air["heapBefore"]=aircraft.heapBefore;air["largestBefore"]=aircraft.largestBefore;air["largestAfterFirst"]=aircraft.largestAfterFirst;air["probeIn"]=aircraft.probeIn;air["probeOut"]=aircraft.probeOut;air["memoryGate"]=aircraft.memoryGate;
 auto ls=d["launches"].to<JsonObject>();ls["valid"]=launches.valid;ls["stale"]=launchStale(launches,now);ls["received"]=int64_t(launches.received);ls["nextAttempt"]=int64_t(launches.nextAttempt);ls["http"]=launches.http;ls["error"]=launches.error;auto le=ls["events"].to<JsonArray>();for(int j=0;j<launches.count;j++){auto e=le.add<JsonObject>();e["name"]=launches.events[j].name;e["net"]=int64_t(launches.events[j].net);e["status"]=launchStatus(launches.events[j]);e["precision"]=launches.events[j].precision;}
 auto ns=d["news"].to<JsonObject>();ns["valid"]=news.valid;ns["stale"]=newsStale(news,now);ns["received"]=int64_t(news.received);ns["http"]=news.http;ns["error"]=news.error;auto ni=ns["items"].to<JsonArray>();for(int j=0;j<news.count;j++){auto e=ni.add<JsonObject>();e["title"]=news.items[j].title;e["link"]=news.items[j].link;e["at"]=int64_t(news.items[j].at);}
 auto qs=d["quake"].to<JsonObject>();qs["valid"]=quake.valid;qs["count"]=quake.count;qs["received"]=int64_t(quake.received);qs["generated"]=int64_t(quake.generated);qs["stale"]=quakeStale(quake,now);qs["http"]=quake.http;qs["error"]=quake.error;
 auto is=d["iss"].to<JsonObject>();is["valid"]=iss.valid;is["stale"]=issStale(iss,now);is["positionAt"]=int64_t(iss.positionAt);is["received"]=int64_t(iss.received);is["http"]=iss.http;is["error"]=iss.error;if(iss.valid){auto look=issLook(iss,cfg.lat,cfg.lon);is["distance"]=look.distance;is["azimuth"]=look.azimuth;is["elevation"]=look.elevation;is["altitude"]=iss.altitude;is["speed"]=iss.speed;}
 d["screenCount"]=VIEW_COUNT-2;auto ids=d["screenIds"].to<JsonArray>();for(int i=0;i<VIEW_COUNT;i++)if(i!=2&&i!=4)ids.add(i);
 auto cc=d["currencies"].to<JsonArray>();for(auto code:CURRENCIES)cc.add(code);
 auto fd=d["finance"].to<JsonArray>();for(int i=0;i<4;i++){const Quote&q=i==0?finance.btc:finance.fx[i-1];auto item=fd.add<JsonObject>();item["valid"]=q.valid;item["heapBefore"]=q.heapBefore;item["largestBefore"]=q.largestBefore;item["largestAfterFirst"]=q.largestAfterFirst;item["probeIn"]=q.probeIn;item["probeOut"]=q.probeOut;item["memoryGate"]=q.memoryGate;item["http"]=q.http;item["tls"]=q.tls;item["memoryDeferred"]=q.memoryDeferred;if(q.valid)item["value"]=q.value;item["received"]=int64_t(q.received);item["date"]=q.date;item["error"]=q.error;item["stale"]=quoteStale(q,now,i==0);}
 d["boardId"]=boardID;d["deviceLabel"]=deviceLabel(cfg,deviceSuffix);d["hostname"]=hostName;d["setupSSID"]=apName;d["token"]=token;d["wifi"]=WiFi.status()==WL_CONNECTED;d["ssid"]=WiFi.status()==WL_CONNECTED?WiFi.SSID():String(cfg.ssid);d["ip"]=WiFi.localIP().toString();d["setup"]=apActive;d["timeValid"]=timeValid(now);d["epoch"]=int64_t(now);d["ntpLast"]=int64_t(ntpLast);d["weatherError"]=weatherError;d["weatherAge"]=weather.valid?int(now-weather.received):-1;d["weatherAt"]=int64_t(weather.received);d["weatherStale"]=weatherStale(weather,now);d["heap"]=ESP.getFreeHeap();d["largestBlockNow"]=heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);d["minHeap"]=ESP.getMinFreeHeap();d["loopStackFree"]=uxTaskGetStackHighWaterMark(nullptr);d["weatherBusy"]=diagnostics.busy;d["weatherAttempt"]=int64_t(diagnostics.attempt);d["weatherHTTP"]=diagnostics.http;d["weatherTLS"]=diagnostics.tls;d["weatherBytes"]=diagnostics.bytes;d["weatherDuration"]=diagnostics.duration;d["largestBlock"]=diagnostics.largestBlock;d["heapBeforeTLS"]=diagnostics.heapBeforeTLS;d["networkStackFree"]=diagnostics.networkStackFree;d["trustProfile"]=diagnostics.trustProfile;d["coreVersion"]=ESP_ARDUINO_VERSION_STR;String s;serializeJson(d,s);server.sendHeader("Cache-Control","no-store");server.send(200,"application/json",s);});
 server.on("/api/view",HTTP_POST,[]{JsonDocument d;if(!body(d))return;if(!d["view"].is<int>()||d["view"].as<int>()<0||d["view"].as<int>()>=VIEW_COUNT){reply(400,"Ansicht muss 0 bis 16 sein");return;}cfg.view=mergedScreenView(d["view"]);manualUntil=millis()+60000;eventView=eventPriority.active=-1;lastSwitch=millis();changed();firstDraw=true;reply(200,"Ansicht gewaehlt");});
 server.on("/api/settings",HTTP_POST,[]{JsonDocument d;if(!body(d))return;Config next=cfg;if(!decodeConfig(d,next)){reply(400,"Ort, Koordinaten, Datum, Intervall und Wechselansichten pruefen.");return;}bool wc=next.lat!=cfg.lat||next.lon!=cfg.lon;if(!financeConfigEqual(cfg,next))finance=FinanceData();if(!aircraftConfigEqual(cfg,next))aircraft=AircraftData();if(next.newsFeed!=cfg.newsFeed){news=NewsData();news.source=next.newsFeed;}cfg=next;if(wc){weather=Weather();weatherError="Warte auf Wetterdaten";generation++;}lastSwitch=millis();changed();firstDraw=true;reply(200,"Gespeichert; Flash nach 3 Sekunden.");});
 server.on("/api/device",HTTP_POST,[]{JsonDocument d;if(!body(d))return;if(otaActive){reply(409,"Update laeuft");return;}Config next=cfg;if(!textValue(d,"deviceName",next.deviceName,sizeof(next.deviceName))){reply(400,"Name zu lang oder ungueltig. Bitte kuerzer eingeben.");return;}if(!strcmp(next.deviceName,cfg.deviceName)){reply(200,"Name unveraendert");return;}cfg=next;saveConfig();reply(200,"Gespeichert. Neustart. Neue Adresse: http://"+deviceHost(cfg,deviceSuffix)+".local/ ; Einrichtungs-WLAN: "+deviceSSID(cfg,deviceSuffix));rebootAt=millis()+2000;});
 server.on("/api/refresh",HTTP_POST,[]{if(!authorized())return;if(otaActive){reply(409,"Update laeuft");return;}uint32_t now=millis();if(lastManualRefresh&&uint32_t(now-lastManualRefresh)<10000){reply(429,"Bitte 10 Sekunden warten.");return;}lastManualRefresh=now;refreshRequest++;sendConfig();reply(200,"Wetterabruf angefordert. Ergebnisse erscheinen unten.");});
 server.on("/api/scan",HTTP_POST,[]{if(!authorized())return;if(otaActive){reply(409,"Update laeuft");return;}if(!scanRunning)requestScan();reply(200,"WLAN-Suche gestartet");});
 server.on("/api/scan",HTTP_GET,[]{JsonDocument d;d["running"]=scanRunning||scanRequested;d["done"]=scanDone;d["error"]=scanError;auto a=d["networks"].to<JsonArray>();for(int i=0;i<networkCount;i++){auto n=a.add<JsonObject>();n["ssid"]=networks[i].ssid;n["rssi"]=networks[i].rssi;n["secure"]=networks[i].secure;}String s;serializeJson(d,s);server.sendHeader("Cache-Control","no-store");server.send(200,"application/json",s);});
 server.on("/api/setup",HTTP_POST,[]{if(!authorized())return;if(otaActive){reply(409,"Update laeuft");return;}startAP();showSetupUntil=millis()+60000;reply(200,"Einrichtungs-WLAN aktiv. Passwort siehe Display.");firstDraw=true;});
 server.on("/frame.bmp",HTTP_GET,[]{
  // 4-bit indexed BMP, same framebuffer as the physical display.
  const uint32_t offset=14+40+16*4,total=offset+120*240;uint8_t h[54]={0};h[0]='B';h[1]='M';auto put=[&](int p,uint32_t n){for(int i=0;i<4;i++)h[p+i]=(n>>(8*i))&255;};put(2,total);put(10,offset);put(14,40);put(18,240);put(22,240);h[26]=1;h[28]=4;put(34,28800);put(46,16);
  server.sendHeader("Cache-Control","no-store");server.setContentLength(total);server.send(200,"image/bmp","");auto client=server.client();uint32_t started=millis();if(!frameWrite(client,h,54,started)){client.stop();return;}uint8_t pal[64]={};for(int i=0;i<16;i++){uint16_t c=activePalette[i];pal[4*i]=(c&31)*255/31;pal[4*i+1]=((c>>5)&63)*255/63;pal[4*i+2]=((c>>11)&31)*255/31;}if(!frameWrite(client,pal,64,started)){client.stop();return;}for(int y=239;y>=0;y--){if(!frameWrite(client,canvas.getBuffer()+120*y,120,started)){client.stop();return;}delay(0);}
 });
 wifiRoutes();boardRoutes();otaRoutes();systemRoutes();
 server.onNotFound([]{server.sendHeader("Location","/");server.send(302,"text/plain","");});server.begin();
}
void setup(){
 Serial.begin(115200);setenv("TZ","CET-1CEST,M3.5.0,M10.5.0/3",1);tzset();
 SPI.begin(TFT_SCL,-1,TFT_SDA,TFT_CS);display.begin(40000000);display.setRotation(0);
 if(!canvas.getBuffer()){display.fillScreen(0);while(true)delay(1000);}
 prefs.begin("runde-uhr",false);String raw=prefs.getString("config","{}");JsonDocument d;if(!deserializeJson(d,raw)){bool migrated=false;bool ok=loadConfig(d,cfg,migrated);if(ok&&migrated&&raw!="{}"){if(!prefs.isKey("config-pre-merge"))prefs.putString("config-pre-merge",raw);if(!prefs.isKey("config-pre4"))prefs.putString("config-pre4",raw);if(!prefs.isKey("config-pre3"))prefs.putString("config-pre3",raw);if(!prefs.isKey("config-v1"))prefs.putString("config-v1",raw);saveConfig();}}
 apKey=prefs.getString("apkey","");if(apKey.length()!=12){const char* a="23456789ABCDEFGHJKLMNPQRSTUVWXYZ";apKey="";for(int i=0;i<12;i++)apKey+=a[esp_random()%strlen(a)];prefs.putString("apkey",apKey);}
 uint64_t mac=ESP.getEfuseMac();char id[8];snprintf(id,sizeof(id),"%04X",unsigned(mac&65535));deviceSuffix=id;hostName=deviceHost(cfg,deviceSuffix);apName=deviceSSID(cfg,deviceSuffix);token=String(esp_random(),HEX)+String(esp_random(),HEX);
 char fullId[17];snprintf(fullId,sizeof(fullId),"%012llX",(unsigned long long)(mac&0xffffffffffffULL));boardID=fullId;loadWifi();
 WiFi.persistent(false);WiFi.setHostname(hostName.c_str());WiFi.mode(WIFI_STA);WiFi.setAutoReconnect(false);startAP();if(savedCount)connectSaved(0,millis());requestScan();
 sntp_set_time_sync_notification_cb(onTimeSync);
 configTzTime("CET-1CEST,M3.5.0,M10.5.0/3","pool.ntp.org","time.cloudflare.com");
 newsResultQueue=xQueueCreate(1,sizeof(NewsResult));launchResultQueue=xQueueCreate(1,sizeof(LaunchResult));quakeResultQueue=xQueueCreate(1,sizeof(QuakeResult));issResultQueue=xQueueCreate(1,sizeof(IssResult));aircraftResultQueue=xQueueCreate(1,sizeof(AircraftResult));financeResultQueue=xQueueCreate(1,sizeof(FinanceResult));netConfigQueue=xQueueCreate(1,sizeof(NetConfig));netResultQueue=xQueueCreate(1,sizeof(NetResult));if(!newsResultQueue||!launchResultQueue||!quakeResultQueue||!issResultQueue||!aircraftResultQueue||!financeResultQueue||!netConfigQueue||!netResultQueue){display.fillScreen(0);while(true)delay(1000);}sendConfig();if(xTaskCreate(networkTask,"network",12288,nullptr,1,nullptr)!=pdPASS){weatherError="Netzwerk-Task: RAM fehlt";}routes();offlineSince=millis();
}
void loop(){
 uint32_t ms=millis();if(rebootAt&&int32_t(ms-rebootAt)>=0){ESP.restart();return;}bool online=WiFi.status()==WL_CONNECTED;
 if(online&&!wasConnected){MDNS.end();MDNS.begin(hostName.c_str());MDNS.addService("rundeuhr","tcp",80);MDNS.addServiceTxt("rundeuhr","tcp","name",deviceLabel(cfg,deviceSuffix));Serial.print("RundeUhr IP: ");Serial.println(WiFi.localIP());offlineSince=ms;}if(!online&&wasConnected)offlineSince=ms;wasConnected=online;
 if(!online&&uint32_t(ms-offlineSince)>45000)startAP();
 if(!otaActive)tickScan(ms);tickSavedWifi(ms);tickBoards(ms);tickOTA(ms);
 if(!online&&uint32_t(ms-offlineSince)>45000&&!scanRunning&&!scanRequested&&(!scanDone||uint32_t(ms-scanFinished)>60000))requestScan();
 if(wifiPending&&!scanRunning){wifiPending=false;scanRequested=false;connectSaved(wifiChoice,ms);}
 if(apActive)dns.processNextRequest(); // Keep the named, password-protected WLAN visible, also when connected to home Wi-Fi.
 server.handleClient();NetResult& r=*new (&receiveScratch.weather) NetResult;while(xQueueReceive(netResultQueue,&r,0)==pdTRUE){if(r.generation!=generation)continue;weatherError=r.error;diagnostics=r.diagnostics;if(!diagnostics.busy){Serial.print("Wetter: ");Serial.println(weatherError.length()?weatherError:"OK");}if(r.weather.valid)weather=r.weather;firstDraw=true;}
 FinanceResult& fr=*new (&receiveScratch.finance) FinanceResult;while(xQueueReceive(financeResultQueue,&fr,0)==pdTRUE){if(financeConfigEqual(cfg,fr.config)){finance=fr.data;firstDraw=true;}}
 NewsResult& nr=*new (&receiveScratch.news) NewsResult;while(xQueueReceive(newsResultQueue,&nr,0)==pdTRUE){if(nr.data.source==cfg.newsFeed){news=nr.data;firstDraw=true;}}
 LaunchResult& lr=*new (&receiveScratch.launch) LaunchResult;while(xQueueReceive(launchResultQueue,&lr,0)==pdTRUE){launches=lr.data;firstDraw=true;}
 QuakeResult& qr=*new (&receiveScratch.quake) QuakeResult;while(xQueueReceive(quakeResultQueue,&qr,0)==pdTRUE){quake=qr.data;firstDraw=true;}
 IssResult& ir=*new (&receiveScratch.iss) IssResult;while(xQueueReceive(issResultQueue,&ir,0)==pdTRUE){iss=ir.data;firstDraw=true;}
 AircraftResult& ar=*new (&receiveScratch.aircraft) AircraftResult;while(xQueueReceive(aircraftResultQueue,&ar,0)==pdTRUE){if(aircraftConfigEqual(cfg,ar.config)){aircraft=ar.data;firstDraw=true;}}
 if(timeValid(time(nullptr))&&(!moon.valid||uint32_t(ms-lastMoon)>60000)){lastMoon=ms;moon=calculateMoon(time(nullptr));}
 bool manual=manualUntil&&int32_t(manualUntil-ms)>0;Config policy=cfg;if(manual)policy.automatic=false;
 int previousEvent=eventView;eventView=eventPriority.tick(policy,launches,quake,news,time(nullptr),ms,online);if(previousEvent!=eventView){firstDraw=true;lastSwitch=ms;}
 if(manual||eventView>=0||aircraftPriorityActive(cfg,aircraft,time(nullptr),online))lastSwitch=ms;
 else if(cfg.automatic){auto mask=eventCycleMask(cfg,launches,quake,news,time(nullptr),online)&aircraftCycleMask(cfg);if(!(mask&(1UL<<cfg.view))||uint32_t(ms-lastSwitch)>=(cfg.view==16&&news.count?std::max(uint32_t(cfg.interval*1000UL),newsCycleDuration(cfg,news,time(nullptr))):uint32_t(cfg.interval*1000UL))){cfg.view=mask?nextView(cfg.view,mask):0;lastSwitch=ms;sendConfig();firstDraw=true;}}
 if(dirty&&!otaActive&&uint32_t(ms-dirtyAt)>3000)saveConfig();
 if(firstDraw||uint32_t(ms-lastDraw)>=(actualView(time(nullptr),online)==16?20U:cfg.view>=5&&cfg.view<=8&&cfg.animations?180U:1000U)){static uint32_t newsFullAt=0;bool partialNews=!firstDraw&&renderedView==16&&actualView(time(nullptr),online)==16&&uint32_t(ms-newsFullAt)<1000;firstDraw=false;lastDraw=ms;Config displayConfig=cfg;if(manual)displayConfig.automatic=false;displayConfig.view=actualView(time(nullptr),online);if(renderedView!=displayConfig.view){renderedView=displayConfig.view;renderSince=ms;}bool financeSetup=apActive&&(!savedCount||(!online&&(uint32_t(ms-apSince)<60000||ms%60000<10000))||(showSetupUntil&&int32_t(showSetupUntil-ms)>0));if(financeSetup||!timeValid(time(nullptr)))partialNews=false;activePalette=palette;if(displayConfig.view==16&&timeValid(time(nullptr))&&!financeSetup)renderNews(canvas,displayConfig,news,time(nullptr),online,ms-renderSince);else if(displayConfig.view==15&&timeValid(time(nullptr))&&!financeSetup)renderLaunch(canvas,displayConfig.automatic?filteredLaunch(cfg,launches,time(nullptr)):launches,time(nullptr),online);else if(displayConfig.view==14&&timeValid(time(nullptr))&&!financeSetup)renderQuake(canvas,displayConfig.automatic?filteredQuake(cfg,quake,time(nullptr)):quake,time(nullptr),online,cfg);else if(displayConfig.view==13&&timeValid(time(nullptr))&&!financeSetup)renderIss(canvas,cfg,iss,time(nullptr),online);else if(displayConfig.view==12&&timeValid(time(nullptr))&&!financeSetup)renderAircraft(canvas,aircraft,time(nullptr),online);else if(displayConfig.view>=10&&timeValid(time(nullptr))&&!financeSetup)renderFinance(canvas,displayConfig,finance,time(nullptr),online);else render(canvas,displayConfig,weather,moon,time(nullptr),online,apActive&&(!savedCount||(!online&&(uint32_t(ms-apSince)<60000||ms%60000<10000))||(showSetupUntil&&int32_t(showSetupUntil-ms)>0)),apName,apKey,ms);if(!partialNews)newsFullAt=ms;static uint16_t row[240];for(int y=partialNews?100:0;y<(partialNews?132:240);y++){for(int x=0;x<240;x++)row[x]=activePalette[canvas.getPixel(x,y)];display.drawRGBBitmap(0,y,row,240,1);}}
 delay(2);
}
