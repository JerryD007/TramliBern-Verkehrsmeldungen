#pragma once
// Cross-origin OTA is limited to IP origins of recently discovered peers on our subnet.
// No wildcard origins and no remote access to settings, Wi-Fi or full diagnostics.
bool fleetOriginAllowed(const String&origin){
 IPAddress local=WiFi.localIP(),mask=WiFi.subnetMask();
 for(const auto&p:peers){
  if(!p.id[0]||uint32_t(millis()-p.seen)>35000)continue;
  bool subnet=true;for(int i=0;i<4;i++)if((p.ip[i]&mask[i])!=(local[i]&mask[i]))subnet=false;
  if(subnet&&(origin==String("http://")+p.ip.toString()||origin==String("http://")+p.ip.toString()+":80"))return true;
 }
 return false;
}
void fleetRoutes(){
 server.addMiddleware([](WebServer&s,Middleware::Callback next)->bool{
  String origin=s.header("Origin");if(origin.isEmpty()||origin==String("http://")+s.hostHeader())return next();
  String path=s.uri();bool pathOK=path=="/api/fleet/state"||path=="/api/update/info"||path=="/api/update/begin"||path=="/api/update/chunk"||path=="/api/update/finish"||path=="/api/update/cancel";
  if(!pathOK||!fleetOriginAllowed(origin)){s.send(403,"application/json","{\"message\":\"Fleet origin denied\"}");return true;}
  s.sendHeader("Access-Control-Allow-Origin",origin);s.sendHeader("Vary","Origin");
  if(s.method()==HTTP_OPTIONS){s.sendHeader("Access-Control-Allow-Methods","GET, POST, OPTIONS");s.sendHeader("Access-Control-Allow-Headers","Content-Type, X-Uhr-Token");s.sendHeader("Access-Control-Max-Age","60");if(s.header("Access-Control-Request-Private-Network")=="true")s.sendHeader("Access-Control-Allow-Private-Network","true");s.send(204);return true;}
  if(s.method()!=HTTP_GET&&s.method()!=HTTP_POST){s.send(405);return true;}return next();
 });
 server.on("/api/fleet/state",HTTP_GET,[]{JsonDocument d;d["fleetProtocol"]=1;d["boardId"]=boardID;d["deviceLabel"]=deviceLabel(cfg,deviceSuffix);d["ip"]=WiFi.localIP().toString();d["firmware"]="3.2.0-rc12";d["bootId"]=token;d["token"]=token;d["imageSHA256"]=runningImageSHA();String s;serializeJson(d,s);server.sendHeader("Cache-Control","no-store");server.send(200,"application/json",s);});
}
