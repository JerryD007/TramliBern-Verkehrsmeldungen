#pragma once
struct SavedWifi {char ssid[33]={},pass[65]={};};
SavedWifi savedWifi[8];int savedCount=0,wifiChoice=-1;uint32_t wifiAttempt=0;bool wifiCycle=false;
void persistWifi(){JsonDocument d;auto a=d.to<JsonArray>();for(int i=0;i<savedCount;i++){auto o=a.add<JsonObject>();o["ssid"]=savedWifi[i].ssid;o["pass"]=savedWifi[i].pass;}String s;serializeJson(d,s);prefs.putString("wlans-v1",s);}
void loadWifi(){String raw=prefs.getString("wlans-v1","");JsonDocument d;
 if(raw.length()&&!deserializeJson(d,raw)&&d.is<JsonArray>())for(auto o:d.as<JsonArray>()){const char*s=o["ssid"]|"",*p=o["pass"]|"";if(savedCount<8&&strlen(s)>0&&strlen(s)<33&&strlen(p)<65){strlcpy(savedWifi[savedCount].ssid,s,33);strlcpy(savedWifi[savedCount++].pass,p,65);}}
 if(!raw.length()&&strlen(cfg.ssid)){strlcpy(savedWifi[0].ssid,cfg.ssid,33);strlcpy(savedWifi[0].pass,cfg.pass,65);savedCount=1;persistWifi();}
}
void connectSaved(int i,uint32_t now){if(i<0||i>=savedCount)return;wifiChoice=i;WiFi.disconnect(false,false);WiFi.begin(savedWifi[i].ssid,savedWifi[i].pass);wifiAttempt=now;}
void tickSavedWifi(uint32_t now){
 if(wifiPending)return;
 if(otaActive||WiFi.status()==WL_CONNECTED){wifiCycle=false;return;}
 if(!savedCount)return;
 if(!wifiCycle){wifiCycle=true;wifiAttempt=now;requestScan();}
 if(scanRunning||scanRequested)return;
 if(uint32_t(now-wifiAttempt)<18000)return;
 int next=(wifiChoice+1)%savedCount; // bounded round-robin includes hidden networks and wrong-password fallbacks
 connectSaved(next,now);requestScan();
}
void wifiRoutes(){
 server.on("/api/wifis",HTTP_GET,[]{JsonDocument d;auto a=d["networks"].to<JsonArray>();for(int i=0;i<savedCount;i++){auto o=a.add<JsonObject>();o["ssid"]=savedWifi[i].ssid;o["connected"]=WiFi.status()==WL_CONNECTED&&WiFi.SSID()==savedWifi[i].ssid;}String s;serializeJson(d,s);server.send(200,"application/json",s);});
 server.on("/api/wifi/remove",HTTP_POST,[]{JsonDocument d;if(!body(d))return;if(otaActive){reply(409,"Update laeuft");return;}String name=d["ssid"]|"";int at=-1;for(int i=0;i<savedCount;i++)if(name==savedWifi[i].ssid)at=i;if(at<0){reply(404,"Netz nicht gespeichert");return;}bool active=WiFi.SSID()==name;for(int i=at;i<savedCount-1;i++)savedWifi[i]=savedWifi[i+1];savedWifi[--savedCount]=SavedWifi();persistWifi();if(name==cfg.ssid){cfg.ssid[0]=cfg.pass[0]=0;saveConfig();}reply(200,"Netz entfernt");if(active){startAP();WiFi.disconnect(false,false);wifiChoice=-1;wifiCycle=false;}});
 server.on("/api/wifi",HTTP_POST,[]{JsonDocument d;if(!body(d))return;if(otaActive){reply(409,"Update laeuft");return;}Config next=cfg;if(!textValue(d,"ssid",next.ssid,33,false)||!textValue(d,"pass",next.pass,65)||(strlen(next.pass)>0&&strlen(next.pass)<8)){reply(400,"SSID 1-32 Bytes; Passwort leer oder 8-64 Bytes.");return;}int at=-1;for(int i=0;i<savedCount;i++)if(!strcmp(savedWifi[i].ssid,next.ssid))at=i;if(at<0){if(savedCount==8){reply(400,"Maximal 8 Netze. Zuerst ein Netz entfernen.");return;}at=savedCount++;}strlcpy(savedWifi[at].ssid,next.ssid,33);strlcpy(savedWifi[at].pass,next.pass,65);persistWifi();cfg=next;saveConfig();reply(200,"Netz gespeichert. Verbindung wird aufgebaut.");startAP();wifiChoice=at;wifiPending=true;});
}
