#pragma once
#include <Update.h>
#include <esp_ota_ops.h>
// Cached once per boot: SHA-256 of the running application image.
String runningImageSHA(){static String value;if(value.length())return value;uint8_t hash[32];if(esp_partition_get_sha256(esp_ota_get_running_partition(),hash)!=ESP_OK)return "";char out[65];for(int i=0;i<32;i++)snprintf(out+2*i,3,"%02x",hash[i]);value=out;return value;}
const char FIRMWARE_FAMILY[]="RUNDEUHR_C3_OTA_LAYOUT_1";
size_t otaSize=0,otaWritten=0;uint32_t otaTouched=0;bool otaFamily=false;String otaTail,otaSession;
void stopOTA(){Update.abort();otaActive=false;otaSize=otaWritten=0;otaTail="";otaSession="";}
void tickOTA(uint32_t ms){if(otaActive&&uint32_t(ms-otaTouched)>30000)stopOTA();}
int hexNibble(char c){if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;return -1;}
// Read the body once; report allocation failures separately from malformed JSON.
bool otaBody(JsonDocument &d){
 if(!authorized())return false;
 String otaRequest=server.arg("plain");
 if(otaRequest.isEmpty()){reply(400,"Update-Anfrage leer. Seite neu laden und erneut versuchen.");return false;}
 if(otaRequest.length()>5000){reply(413,"Update-Block zu gross");return false;}
 auto error=deserializeJson(d,otaRequest.c_str(),otaRequest.length());
 if(error){reply(error==DeserializationError::NoMemory?503:400,"Update-JSON: "+String(error.c_str()));return false;}
 if(String(d["target"]|"")!=boardID){reply(409,"Falsches Zielgeraet. Seite neu laden.");return false;}
 return true;
}
void otaRoutes(){
 server.on("/api/update/info",HTTP_GET,[]{JsonDocument d;const esp_partition_t*p=esp_ota_get_next_update_partition(nullptr);const esp_partition_t*r=esp_ota_get_running_partition();d["supported"]=p&&r&&p->address!=r->address;d["capacity"]=p?p->size:0;d["family"]=FIRMWARE_FAMILY;d["busy"]=bool(otaActive);String s;serializeJson(d,s);server.send(200,"application/json",s);});
 server.on("/api/update/begin",HTTP_POST,[]{JsonDocument d;if(!otaBody(d))return;if(otaActive){reply(409,"Update bereits aktiv");return;}const esp_partition_t*p=esp_ota_get_next_update_partition(nullptr);const esp_partition_t*r=esp_ota_get_running_partition();size_t size=d["size"]|0;if(!p||!r||p->address==r->address||size<1024||size>p->size||String(d["family"]|"")!=FIRMWARE_FAMILY){reply(400,"Firmware oder OTA-Partition passt nicht. Erstinstallation per USB erforderlich.");return;}otaActive=true;if(networkBusy){otaActive=false;reply(409,"Wetterabruf laeuft. Bitte kurz warten und erneut starten.");return;}if(!Update.begin(size,U_FLASH)){stopOTA();reply(500,"Update konnte nicht starten");return;}otaSize=size;otaWritten=0;otaFamily=false;otaTail="";otaTouched=millis();otaSession=String(esp_random(),HEX)+String(esp_random(),HEX);JsonDocument out;out["ok"]=true;out["session"]=otaSession;String s;serializeJson(out,s);server.send(200,"application/json",s);});
 server.on("/api/update/chunk",HTTP_POST,[]{JsonDocument d;if(!otaBody(d))return;if(!otaActive||String(d["session"]|"")!=otaSession){reply(409,"Update-Sitzung ungueltig");return;}size_t offset=d["offset"]|size_t(-1);const char*hex=d["hex"]|"";size_t len=strlen(hex);if(offset!=otaWritten||!len||len>4096||len%2||otaWritten+len/2>otaSize){reply(409,"Update-Reihenfolge ungueltig");return;}static uint8_t bytes[2048];for(size_t i=0;i<len/2;i++){int a=hexNibble(hex[2*i]),b=hexNibble(hex[2*i+1]);if(a<0||b<0){reply(400,"Ungueltige Firmwaredaten");return;}bytes[i]=(a<<4)|b;}
 if(otaWritten==0&&(len/2<24||bytes[0]!=0xe9||bytes[12]!=5||bytes[13]!=0)){stopOTA();reply(400,"Keine ESP32-C3 Anwendungs-Firmware");return;}
 // Search across chunk boundaries, without constructing a full firmware buffer.
 for(size_t i=0;i<len/2;i++){otaTail+=char(bytes[i]);if(otaTail.length()>strlen(FIRMWARE_FAMILY))otaTail.remove(0,1);if(otaTail==FIRMWARE_FAMILY)otaFamily=true;}
 if(Update.write(bytes,len/2)!=len/2){stopOTA();reply(500,"Flash-Schreiben fehlgeschlagen; bisherige Firmware bleibt aktiv");return;}otaWritten+=len/2;otaTouched=millis();reply(200,"Block gespeichert");});
 server.on("/api/update/finish",HTTP_POST,[]{JsonDocument d;if(!otaBody(d))return;if(!otaActive||String(d["session"]|"")!=otaSession){reply(409,"Update-Sitzung ungueltig");return;}if(otaWritten!=otaSize||!otaFamily){stopOTA();reply(400,"Firmware unvollstaendig oder kein kompatibles RundeUhr-Projekt");return;}if(!Update.end()){stopOTA();reply(400,"Firmwarepruefung fehlgeschlagen; bisherige Firmware bleibt aktiv");return;}saveConfig();otaActive=false;reply(200,"Update erfolgreich. Neustart in 2 Sekunden.");rebootAt=millis()+2000;});
 server.on("/api/update/cancel",HTTP_POST,[]{JsonDocument d;if(!otaBody(d))return;if(otaActive&&String(d["session"]|"")==otaSession)stopOTA();reply(200,"Update beendet");});
}
