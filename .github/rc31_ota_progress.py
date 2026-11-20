from pathlib import Path
import sys
p=Path(sys.argv[1]);s=p.read_text(encoding="utf-8")
def replace(a,b):
 global s
 assert s.count(a)==1, ("target count",s.count(a),a[:80])
 s=s.replace(a,b,1)
replace('constexpr const char *VERSION = "TramliBern 4.0 RC30 HomeCross";','constexpr const char *VERSION = "TramliBern 4.0 RC31 HomeCross";')
replace('Version 4.0 RC29 HomeCross · 3 Seiten','Version 4.0 RC31 HomeCross · 3 Seiten')
replace("""    h+="<form method='post' action='/ota/upload' enctype='multipart/form-data'><label>Firmware .ino.bin</label><input name='firmware' type='file' accept='.bin' required><button>Firmware hochladen</button></form></section>";sendPage(h);""",r'''    h+=R"OTAPROGRESS(<form id="otaForm" method="post" action="/ota/upload" enctype="multipart/form-data">
<label for="fwFile">Firmware .ino.bin</label><input id="fwFile" name="firmware" type="file" accept=".bin" required>
<button id="otaUpload" type="submit" class="full">Firmware hochladen</button></form>
<div id="otaProgressBox" role="region" aria-label="OTA Fortschritt" style="display:none;margin-top:20px">
<div id="otaPercent" style="font-size:26px;font-weight:800;color:#086b62" aria-live="off">0 %</div>
<div id="otaTrack" role="progressbar" aria-label="Dateiübertragung" aria-valuemin="0" aria-valuemax="100" aria-valuenow="0" style="height:22px;background:#d9e5f0;border:2px solid #9ab2c7;border-radius:12px;overflow:hidden">
<div id="otaFill" style="width:0%;height:100%;background:#008f83;transition:width .12s linear"></div></div>
<p id="otaState" role="status" aria-live="polite">Bereit</p>
<p class="hint">Prozent = Übertragung vom iPhone zum ESP32, nicht die Installation.</p>
<a id="otaBack" href="/ota" class="button full" style="display:none">Erneut versuchen</a></div>
<script>
(function(){
'use strict';
const form=document.getElementById('otaForm'),file=document.getElementById('fwFile'),btn=document.getElementById('otaUpload');
const box=document.getElementById('otaProgressBox'),pct=document.getElementById('otaPercent'),fill=document.getElementById('otaFill'),bar=document.getElementById('otaTrack'),state=document.getElementById('otaState'),again=document.getElementById('otaBack');
let running=false;
function percent(n){const p=Math.max(0,Math.min(100,Math.floor(n)));pct.textContent=p+' %';fill.style.width=p+'%';bar.setAttribute('aria-valuenow',String(p));}
function fail(msg){running=false;btn.disabled=false;file.disabled=false;state.textContent=msg;again.style.display='inline-flex';}
form.addEventListener('submit',function(e){
if(!window.XMLHttpRequest||!window.FormData)return;
e.preventDefault();if(running)return;
const chosen=file.files&&file.files[0];if(!chosen){state.textContent='Bitte eine Firmware-Datei auswählen.';return;}
const name=chosen.name.toLowerCase();
if(!name.endsWith('.ino.bin')){box.style.display='block';fail('Nur passende .ino.bin auswählen – keine merged.bin.');return;}
if(chosen.size>1966080||chosen.size<65536){box.style.display='block';fail('Firmware-Größe ungültig (max. 1.966.080 Byte).');return;}
running=true;box.style.display='block';again.style.display='none';btn.disabled=true;file.disabled=true;percent(0);
state.textContent='Firmware wird vom iPhone an TramliBern übertragen …';
const data=new FormData();data.append('firmware',chosen,chosen.name);
const req=new XMLHttpRequest();req.open('POST','/ota/upload',true);req.timeout=180000;
req.upload.onprogress=function(event){if(event.lengthComputable){percent(event.loaded/event.total*100);if(event.loaded>=event.total)state.textContent='Übertragung beendet. ESP32 prüft und installiert die Firmware …';}};
req.onload=function(){if(req.status===200&&req.responseText.indexOf('Firmware erfolgreich geladen')>=0){percent(100);state.textContent='Firmware angenommen. ESP32 startet neu. Danach die Verbindung erneut öffnen.';again.style.display='none';}else{fail('Firmware-Update fehlgeschlagen (HTTP '+req.status+'). Alte Version überprüfen.');}};
req.onerror=function(){fail('Verbindung unterbrochen. Version am Display prüfen, bevor erneut geladen wird.');};
req.ontimeout=function(){fail('Zeitüberschreitung. Version am Display überprüfen.');};
req.send(data);
});})();
</script></section>)OTAPROGRESS";
    sendPage(h);''')
replace('    delay(200);ESP.restart();','    delay(900);ESP.restart();')
assert 'otaFromHomeWifi()' in s and 'showWifiQR()' in s and 'setScreenSleep' in s
assert s.count('req.upload.onprogress')==1 and s.count('Update.begin(')==1
p.write_text(s,encoding="utf-8")
print("RC31 OTA upload progress patch applied")
