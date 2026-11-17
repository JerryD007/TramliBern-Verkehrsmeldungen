'use strict';
const $=id=>document.getElementById(id);
const demo=location.protocol==='file:'||new URLSearchParams(location.search).has('demo');
let state=null,token='',uploading=false,online=false,inFlight=false,loaded=false,frameBusy=false,frameUrl='',scanTimer=null,activePage='overview';
const names=['Analoguhr','Wetter & Regen','Wetter & Regen','Sonne & Mond','Sonne & Mond','Ferien','Geburtstag','Wochenende','Arschloch weg','Frei wählbar','Bitcoin','Währungen','Nächstes Flugzeug','ISS Radar','Erdbeben','Raketenstarts','Nachrichten'];
const days=['Sonntag','Montag','Dienstag','Mittwoch','Donnerstag','Freitag','Samstag'];
const configKeys=['newsFeed','launchPriority','quakePriority','launchWindow','quakeWindow','quakeMagnitude','quakeRadius','newsWindow','weekendTransition','aircraftInRange','aircraftPriority','aircraftPlace','aircraftLat','aircraftLon','aircraftRadius','btcCurrency','fxBase','fxQuote','schema','city','lat','lon','event','date','view','mask','interval','automatic','countdownType','countdownDates','customEvent','animations','weekendStartDay','weekendEndDay','weekendStartMinute','weekendEndMinute'];
function notify(text){$('message').textContent=text;$('message').hidden=false;clearTimeout(notify.timer);notify.timer=setTimeout(()=>$('message').hidden=true,9000);}
async function request(path,opts={},timeout=12000){const controller=new AbortController(),timer=setTimeout(()=>controller.abort(),timeout);try{const response=await fetch(path,{cache:'no-store',...opts,signal:controller.signal});const data=await response.json();if(!response.ok){const err=Error(data.message||'Gerät hat die Anfrage abgelehnt ('+response.status+').');err.httpStatus=response.status;throw err;}return data;}catch(e){if(e.name==='AbortError')throw Error('Zeitüberschreitung. Das Ergebnis der Anfrage ist nicht bestätigt.');if(e instanceof TypeError)throw Error('Verbindung unterbrochen. Lokales WLAN prüfen und Safari offen lassen.');throw e;}finally{clearTimeout(timer);}}
async function post(path,data={},timeout){if(demo){if(path==='/api/settings')Object.assign(state,data);if(path==='/api/view')state.view=data.view;notify('Vorschau: Es wurde nichts an ein Gerät gesendet.');return {ok:true,message:'Beispieleinstellungen geändert.'};}if(!token||!online)throw Error('Gerät nicht verbunden. Bitte auf die Verbindung warten.');return request(path,{method:'POST',headers:{'Content-Type':'application/json','X-Uhr-Token':token},body:JSON.stringify(data)},timeout);}
function configSnapshot(){return Object.fromEntries(configKeys.filter(k=>state[k]!==undefined).map(k=>[k,Array.isArray(state[k])?[...state[k]]:state[k]]));}
let saving=false;
async function savePatch(patch){if(!state)throw Error('Zuerst mit der Uhr verbinden.');if(uploading||saving)throw Error('Bitte warten, bis der vorherige Vorgang abgeschlossen ist.');saving=true;try{const next={...configSnapshot(),...patch};const result=await post('/api/settings',next);Object.assign(state,next);if('newsFeed' in patch)state.news=null;if(["aircraftPlace","aircraftLat","aircraftLon","aircraftRadius"].some(k=>k in patch))state.aircraft=null;if(["btcCurrency","fxBase","fxQuote"].some(k=>k in patch))state.finance=[];paint();notify(result.message||'Einstellungen gespeichert.');}finally{saving=false;}}
function safeAction(fn){return async e=>{try{await fn(e);}catch(err){notify(err.message);}};}
function size(n){return Number.isFinite(n)?Math.round(n/1024).toLocaleString('de-CH')+' KiB':'—';}
function timeText(min){return String(Math.floor(min/60)).padStart(2,'0')+':'+String(min%60).padStart(2,'0');}
function minutes(value){const [h,m]=value.split(':').map(Number);return h*60+m;}
function stamp(n){return n?new Intl.DateTimeFormat('de-CH',{timeZone:'Europe/Zurich',day:'2-digit',month:'2-digit',hour:'2-digit',minute:'2-digit'}).format(new Date(n*1000)):'—';}
function facts(id,rows){$(id).replaceChildren(...rows.map(([label,value])=>{const row=document.createElement('div'),dt=document.createElement('dt'),dd=document.createElement('dd');dt.textContent=label;dd.textContent=value??'—';row.append(dt,dd);return row;}));}
function privateIP(value){let a=value.split('.');if(a.length!==4||a.some(x=>!/^(0|[1-9]\d{0,2})$/.test(x)||+x>255))return false;return +a[0]===10||(+a[0]===192&&+a[1]===168)||(+a[0]===172&&+a[1]>=16&&+a[1]<=31);}
function boardURL(host){host=host.trim().replace(/^http:\/\//,'').replace(/\/$/,'');if(privateIP(host)||/^[a-z0-9-]+\.local$/i.test(host))return 'http://'+host+'/';throw Error('Lokale IP-Adresse oder einen Namen mit .local eingeben.');}

// Reload embedded previews once when the connected device changes firmware.
let observedFirmware='',firmwareReloadPending=false;
function refreshForFirmware(version){
 if(!version)return false;
 if(!observedFirmware){observedFirmware=version;return false;}
 if(version===observedFirmware||uploading||saving)return false;
 if(firmwareReloadPending)return true;
 firmwareReloadPending=true;const url=new URL(location.href);url.searchParams.set('fw',version);location.replace(url.href);return true;
}
window.addEventListener('pageshow',event=>{if(event.persisted&&!demo)poll();});
