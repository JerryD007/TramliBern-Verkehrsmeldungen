#pragma once
// Arduino-ESP32 3.3.0: preserve verification flags BEFORE failed TLS is freed.
// PlainStart delays only the handshake. No HTTP bytes may be sent until success.
class IssTlsClient : public WiFiClientSecure {
public:
 bool connectVerified(const char* host,uint16_t port,IssData& out){
  setPlainStart();
  bool initialized=WiFiClientSecure::connect(host,port,5000);
  uint32_t flags=0;int ret;
  if(initialized){
   ret=ssl_starttls_handshake(sslclient.get());
   flags=mbedtls_ssl_get_verify_result(&sslclient->ssl_ctx);
   // Never accept a handshake with nonzero verification flags.
   if(ret>=0&&flags)ret=MBEDTLS_ERR_X509_CERT_VERIFY_FAILED;
   sslclient->last_error=ret<0?ret:0;
  }else{char detail[96]={};ret=lastError(detail,sizeof(detail));if(ret>=0)ret=-1;}
  out.tlsCode=ret>=0?0:ret;
  if(ret>=0){
   _stillinPlainStart=false;
   snprintf(out.tlsDetail,sizeof(out.tlsDetail),"Zertifikat geprueft; Host api.wheretheiss.at; Flags 0x00000000");
   return true;
  }
  char detail[112]={};
  if(initialized&&flags&&flags!=0xffffffffUL)
   mbedtls_x509_crt_verify_info(detail,sizeof(detail),"",flags);
  else if(ret==-1)snprintf(detail,sizeof(detail),"TCP/Setup oder TLS-Zeitlimit; keine eindeutige Zertifikatsursache");
  else mbedtls_strerror(ret,detail,sizeof(detail));
  for(char* p=detail;*p;p++)if(*p=='\n'||*p=='\r')*p=' ';
  snprintf(out.tlsDetail,sizeof(out.tlsDetail),"%s; TLS %d; Flags 0x%08lx; %s",initialized?"Handshake":"TCP/Setup",ret,(unsigned long)flags,detail);
  snprintf(out.tlsFailure,sizeof(out.tlsFailure),"%s",out.tlsDetail);out.tlsFailureAt=out.attempted;
  stop();return false;
 }
};
