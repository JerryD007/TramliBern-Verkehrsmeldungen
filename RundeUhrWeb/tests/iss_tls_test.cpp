#include <cassert>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <memory>
struct IssData {int tlsCode=0;char tlsDetail[192]={},tlsFailure[192]={};time_t attempted=42,tlsFailureAt=0;};
constexpr int MBEDTLS_ERR_X509_CERT_VERIFY_FAILED=-9984;
struct Context {int ssl_ctx=0,last_error=0;};
int result=1;uint32_t verifyFlags=0;bool initializeOK=true;
int ssl_starttls_handshake(Context*){return result;}
uint32_t mbedtls_ssl_get_verify_result(int*){return verifyFlags;}
void mbedtls_x509_crt_verify_info(char*b,size_t n,const char*,uint32_t){snprintf(b,n,"certificate expired\n");}
void mbedtls_strerror(int,char*b,size_t n){snprintf(b,n,"TLS failure");}
class WiFiClientSecure {
protected:std::shared_ptr<Context> sslclient=std::make_shared<Context>();bool _stillinPlainStart=false;
public:bool stopped=false;void setPlainStart(){_stillinPlainStart=true;}
 int connect(const char*,uint16_t,int){assert(_stillinPlainStart);return initializeOK;}
 int lastError(char*,size_t){return -1;}void stop(){stopped=true;}
 bool plain(){return _stillinPlainStart;}
};
#include "../RundeUhr/IssTlsClient.h"
int main(){
 IssData d;result=-9984;verifyFlags=1;IssTlsClient bad;
 assert(!bad.connectVerified("api.wheretheiss.at",443,d));assert(bad.stopped);assert(d.tlsCode==-9984);assert(strstr(d.tlsFailure,"Flags 0x00000001"));assert(strstr(d.tlsFailure,"expired"));assert(d.tlsFailureAt==42);
 char prior[192];strcpy(prior,d.tlsFailure);result=1;verifyFlags=0;IssTlsClient good;
 assert(good.connectVerified("api.wheretheiss.at",443,d));assert(!good.stopped&&!good.plain());assert(d.tlsCode==0);assert(!strcmp(prior,d.tlsFailure));
 result=1;verifyFlags=8;IssTlsClient flags;assert(!flags.connectVerified("api.wheretheiss.at",443,d));assert(flags.stopped);
 initializeOK=false;IssTlsClient setup;assert(!setup.connectVerified("api.wheretheiss.at",443,d));assert(setup.stopped);assert(strstr(d.tlsDetail,"TCP/Setup"));
 puts("PASS ISS TLS failure flags, closed failure, verified success, persistent last failure, TCP failure (host mock)");
}
