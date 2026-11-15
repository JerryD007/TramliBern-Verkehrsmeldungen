#pragma once
struct ScanEntry {char ssid[33];int rssi;bool secure;};
ScanEntry networks[24];int networkCount=0;bool scanRunning=false,scanRequested=false,scanDone=false,wifiPending=false;
uint32_t scanAt=0,scanFinished=0;String scanError="";
void requestScan(){scanRequested=true;}
void tickScan(uint32_t now){
 if(scanRunning){int n=WiFi.scanComplete();if(n!=WIFI_SCAN_RUNNING){
  scanRunning=false;scanDone=true;scanFinished=now;networkCount=0;
  if(n<0)scanError="Suche fehlgeschlagen. Erneut versuchen.";
  else{scanError="";for(int i=0;i<n;i++){
   String name=WiFi.SSID(i);if(!name.length())continue;int found=-1;for(int j=0;j<networkCount;j++)if(name==networks[j].ssid)found=j;
   if(found>=0){if(WiFi.RSSI(i)>networks[found].rssi)networks[found].rssi=WiFi.RSSI(i);continue;}
   if(networkCount<24){auto &e=networks[networkCount++];strlcpy(e.ssid,name.c_str(),sizeof(e.ssid));e.rssi=WiFi.RSSI(i);e.secure=WiFi.encryptionType(i)!=WIFI_AUTH_OPEN;}
  }
  for(int i=0;i<networkCount;i++)for(int j=i+1;j<networkCount;j++)if(networks[j].rssi>networks[i].rssi){auto a=networks[i];networks[i]=networks[j];networks[j]=a;}}
  WiFi.scanDelete();
 }}
 if(scanRequested&&!scanRunning&&!wifiPending){scanRequested=false;scanAt=now;scanError="";WiFi.scanDelete();int r=WiFi.scanNetworks(true,false,false,120);scanRunning=r==WIFI_SCAN_RUNNING;if(!scanRunning){scanError="Suche konnte nicht starten";scanDone=true;scanFinished=now;}}
}
