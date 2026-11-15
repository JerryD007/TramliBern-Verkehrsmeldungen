#pragma once
#include <WiFiUdp.h>
WiFiUDP boardUdp;bool udpReady=false;uint32_t lastHello=0;
struct Peer {char id[17]={},name[49]={};IPAddress ip;uint32_t seen=0;};Peer peers[12];
String boardID;
void tickBoards(uint32_t now){
 if(WiFi.status()!=WL_CONNECTED){if(udpReady)boardUdp.stop();udpReady=false;return;}
 if(!udpReady){udpReady=boardUdp.begin(42173);lastHello=0;}
 for(int k=0;k<3;k++){int size=boardUdp.parsePacket();if(size<=0)break;if(size>300){boardUdp.flush();continue;}char buf[301];int n=boardUdp.read(buf,300);if(n<=0)continue;buf[n]=0;JsonDocument d;if(deserializeJson(d,buf,n)||strcmp(d["kind"]|"","rundeuhr-1"))continue;const char* id=d["id"]|"",*name=d["name"]|"";if(strlen(id)!=12||strlen(name)>48||!strlen(name)||boardID==id)continue;bool valid=true;for(int j=0;j<12;j++)if(!isxdigit(id[j]))valid=false;if(!valid)continue;int at=-1;for(int i=0;i<12;i++)if(!strcmp(peers[i].id,id))at=i;if(at<0)for(int i=0;i<12;i++)if(!peers[i].id[0]||uint32_t(now-peers[i].seen)>60000){at=i;break;}if(at<0)continue;strlcpy(peers[at].id,id,17);strlcpy(peers[at].name,name,49);peers[at].ip=boardUdp.remoteIP();peers[at].seen=now;}
 if(!lastHello||uint32_t(now-lastHello)>10000){lastHello=now;JsonDocument d;d["kind"]="rundeuhr-1";d["id"]=boardID;d["name"]=deviceLabel(cfg,deviceSuffix);String s;serializeJson(d,s);IPAddress broadcast=WiFi.localIP();IPAddress mask=WiFi.subnetMask();for(int i=0;i<4;i++)broadcast[i]|=~mask[i];boardUdp.beginPacket(broadcast,42173);boardUdp.write((const uint8_t*)s.c_str(),s.length());boardUdp.endPacket();}
}
void boardRoutes(){server.on("/api/boards",HTTP_GET,[]{JsonDocument d;auto a=d["boards"].to<JsonArray>();auto me=a.add<JsonObject>();me["id"]=boardID;me["name"]=deviceLabel(cfg,deviceSuffix);me["ip"]=WiFi.localIP().toString();me["self"]=true;me["online"]=true;for(auto&p:peers)if(p.id[0]){auto o=a.add<JsonObject>();o["id"]=p.id;o["name"]=p.name;o["ip"]=p.ip.toString();o["online"]=uint32_t(millis()-p.seen)<35000;}String s;serializeJson(d,s);server.sendHeader("Cache-Control","no-store");server.send(200,"application/json",s);});}
