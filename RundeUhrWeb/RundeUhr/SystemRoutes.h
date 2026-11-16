#pragma once
// Backup intentionally excludes passwords and network lists. Restore leaves both untouched.
void systemRoutes(){
 server.on("/api/config/export",HTTP_POST,[]{
  if(!authorized())return;JsonDocument d;configJSON(d);d.remove("ssid");d["backupFamily"]="RundeUhr-settings-v1";
  String s;serializeJson(d,s);server.sendHeader("Cache-Control","no-store");server.send(200,"application/json",s);
 });
 server.on("/api/config/restore",HTTP_POST,[]{
  JsonDocument d;if(!body(d))return;
  if(strcmp(d["backupFamily"]|"","RundeUhr-settings-v1")|| (d["schema"]|0)!=4){reply(400,"Inkompatible Sicherung");return;}
  Config next=cfg;if(!decodeConfig(d,next)){reply(400,"Sicherung enthaelt ungueltige Einstellungen");return;}
  // Restore settings, but keep the current device identity and Wi-Fi credentials.
  strlcpy(next.deviceName,cfg.deviceName,sizeof(next.deviceName));
  Config original=cfg;cfg=next;JsonDocument save;configJSON(save,true);String s;serializeJson(save,s);
  if(prefs.putString("config",s)!=s.length()){cfg=original;reply(500,"Einstellungen konnten nicht gespeichert werden");return;}
  dirty=false;finance=FinanceData();aircraft=AircraftData();generation++;weather=Weather();lastSwitch=millis();sendConfig();firstDraw=true;reply(200,"Einstellungen wiederhergestellt. Geraetename und WLAN bleiben erhalten.");
 });
 server.on("/api/restart",HTTP_POST,[]{JsonDocument d;if(!body(d))return;saveConfig();reply(200,"Geraet startet neu");rebootAt=millis()+2000;});
}
