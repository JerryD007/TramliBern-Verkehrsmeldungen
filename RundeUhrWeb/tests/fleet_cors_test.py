from pathlib import Path
import tempfile,subprocess
r=Path(__file__).resolve().parents[1];s=(r/'RundeUhr/FleetCors.h').read_text();s=s[:s.index(' server.on(')]+'}\n'
pre=r'''#include "Arduino.h"
#include <map>
#include <functional>
#include <cassert>
#define isEmpty empty
unsigned now=10000;unsigned millis(){return now;}
struct IPAddress{unsigned char b[4];unsigned char operator[](int i)const{return b[i];}String toString()const{return String(int(b[0]))+"."+String(int(b[1]))+"."+String(int(b[2]))+"."+String(int(b[3]));}};
struct WIFI{IPAddress localIP(){return {{192,168,1,20}};}IPAddress subnetMask(){return {{255,255,255,0}};}}WiFi;
struct Peer{char id[17]={};IPAddress ip{};unsigned seen=0;}peers[12];
enum{HTTP_GET,HTTP_POST,HTTP_OPTIONS,HTTP_DELETE};
struct Middleware{using Callback=std::function<bool()>;};
struct WebServer{std::map<std::string,String>input,output;String path="/api/fleet/state",host="192.168.1.20";int verb=HTTP_GET,code=0;std::function<bool(WebServer&,Middleware::Callback)>fn;
String header(const char*k){return input[k];}String uri(){return path;}String hostHeader(){return host;}int method(){return verb;}void sendHeader(const char*k,String v){output[k]=v;}void send(int c,const char* t="",const char* b=""){code=c;}void addMiddleware(std::function<bool(WebServer&,Middleware::Callback)>f){fn=f;}}server;
'''
post=r'''int main(){strcpy(peers[0].id,"123456789abc");peers[0].ip={{192,168,1,10}};peers[0].seen=now;fleetRoutes();auto check=[](String origin,String path,int verb,bool pass,int code){server.input["Origin"]=origin;server.path=path;server.verb=verb;server.code=0;server.output.clear();bool called=false;server.fn(server,[&](){called=true;return true;});assert(called==pass);assert(server.code==code);};
check("","/api/settings",HTTP_POST,true,0);check("http://192.168.1.20","/api/settings",HTTP_POST,true,0);
check("http://192.168.1.10","/api/fleet/state",HTTP_GET,true,0);assert(server.output["Access-Control-Allow-Origin"]=="http://192.168.1.10");
check("http://192.168.1.10","/api/update/chunk",HTTP_OPTIONS,false,204);assert(server.output["Access-Control-Allow-Headers"]=="Content-Type, X-Uhr-Token");
check("https://evil.example","/api/fleet/state",HTTP_GET,false,403);assert(server.output.empty());
check("http://192.168.1.10","/api/state",HTTP_GET,false,403);check("http://192.168.1.10","/api/settings",HTTP_POST,false,403);
now+=35001;check("http://192.168.1.10","/api/fleet/state",HTTP_GET,false,403);peers[0].seen=now;peers[0].ip={{192,168,2,10}};check("http://192.168.2.10","/api/fleet/state",HTTP_GET,false,403);
puts("PASS actual firmware middleware: known fresh same-subnet peer only; explicit CORS, preflight, normal same-origin operations; untrusted/expired/off-subnet peers and settings/diagnostics blocked");}
'''
with tempfile.TemporaryDirectory() as t:
 p=Path(t)/'test.cpp';p.write_text(pre+s+post);subprocess.run(['g++','-std=c++17','-I',str(r/'tests/host'),str(p),'-o',t+'/test'],check=True);subprocess.run([t+'/test'],check=True)
