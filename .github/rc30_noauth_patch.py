from pathlib import Path
import sys
p=Path(sys.argv[1]); s=p.read_text(encoding="utf-8")
def replace(old,new,label):
 global s
 count=s.count(old)
 assert count==1, f"{label}: expected exactly once, got {count}"
 s=s.replace(old,new,1)
replace('constexpr const char *VERSION = "TramliBern 4.0 RC29 HomeCross";','constexpr const char *VERSION = "TramliBern 4.0 RC30 HomeCross";','version')
replace('String otaPassword;\n','','password global')
replace('''bool otaAuthorized() {
  if (otaPassword.length() < 10) {server.send(403,"text/plain","OTA neni aktivovano: nastavte heslo v Nastaveni.");return false;}
  if (!server.authenticate("admin", otaPassword.c_str())) { server.requestAuthentication(); return false; }
  return true;
}''','''// Password-free OTA only through the connected STA Wi-Fi, never through the setup AP.
bool otaFromHomeWifi() {
  return WiFi.status() == WL_CONNECTED && server.client().localIP() == WiFi.localIP();
}''','authentication function')
replace('  otaPassword = prefs.getString("otaPass", "");\n','','credential load')
replace('''    if(otaPassword.length()<10)h+="<p class='notice'>OTA deaktiviert. Zuerst Passwort mit mindestens 10 Zeichen setzen.</p>";
    else h+="<p>OTA aktiviert. Benutzername: admin. Safari fragt beim Upload nach dem OTA-Passwort.</p>";
    h+="<form method='post' action='/ota/password'><label>OTA-Passwort setzen/ersetzen (mind. 10 Zeichen)</label><input name='pass' type='password' minlength='10' maxlength='63' required autocomplete='new-password'><button>Passwort speichern</button></form>";''','''    h+="<p>OTA ohne Benutzername und Passwort, nur ueber verbundenes Heim-WLAN. Nicht ueber das Setup-WLAN verfuegbar.</p>";''','OTA form')
replace('''  server.on("/ota/password",HTTP_POST,[](){
    String pw=server.arg("pass");if(pw.length()<10||pw.length()>63){showMessage("Passwort: 10 bis 63 Zeichen",400);return;}
    if(otaPassword.length()>=10 && !server.authenticate("admin",otaPassword.c_str())){server.requestAuthentication();return;}
    otaPassword=pw;prefs.putString("otaPass",pw);server.sendHeader("Location","/ota");server.send(303);
  });
''','','password endpoint')
replace('''    if(!otaAuthorized())return;
    if(!otaSuccess''','''    if(!otaFromHomeWifi()) {server.send(403,"text/plain","OTA nur ueber verbundenes Heim-WLAN");return;}
    if(!otaSuccess''','OTA handler')
replace('''    // Callback may be invoked before the final request handler. Never write without auth.
    if(otaPassword.length()<10 || !server.authenticate("admin",otaPassword.c_str())) {otaUploadFailed=true;return;}''','''    // Upload callback runs before final handler. Never flash from the setup AP.
    if(!otaFromHomeWifi()) {otaUploadFailed=true;return;}''','OTA upload')
assert 'server.authenticate("admin"' not in s
assert 'otaPassword' not in s
assert 'otaFromHomeWifi' in s and 'showWifiQR()' in s and 'setScreenSleep' in s
p.write_text(s,encoding="utf-8")
print("RC30 password-free OTA patch successful; QR and sleep retained")
