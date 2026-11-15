// The browser owns one file and sends sequential writes directly to each ESP.
let fleetRows=[],fleetStopRequested=false;
function fleetHostReady(){const ok=privateIP(location.hostname);const el=$('fleetOriginHint');el.replaceChildren();if(!ok&&state&&privateIP(state.ip)){const a=document.createElement('a');a.href=boardURL(state.ip)+'#system';a.textContent='Otevřít přes IP adresu pro hromadnou aktualizaci';el.append(a);}return ok;}
function fleetLine(row,text){row.status.textContent=text;}
async function fleetRequest(row,path,body,timeout=12000){const opts=body===undefined?{}:{method:'POST',headers:{'Content-Type':'application/json','X-Uhr-Token':row.remote.token},body:JSON.stringify(body)};return request(row.base+path,opts,timeout);}
async function fleetState(row){const s=await fleetRequest(row,'/api/fleet/state');if(s.fleetProtocol!==1||s.boardId!==row.id||!s.token||!s.bootId)throw Error('Nekompatibilní nebo jiné zařízení. Nejprve nahraj RC6 jednotlivě.');return s;}
$('fleetRefresh').onclick=safeAction(async()=>{
 if(uploading)return;if(demo)throw Error('Hromadná aktualizace není dostupná v ukázce.');if(!fleetHostReady())throw Error('Nejprve otevři hodiny přes odkaz s IP adresou.');
 const d=await request('/api/boards'),seen=new Set();fleetRows=[];$('fleetTargets').replaceChildren();
 for(const b of d.boards){if(!/^[0-9a-f]{12}$/i.test(b.id||'')||!privateIP(b.ip)||seen.has(b.id))continue;seen.add(b.id);const row={...b,base:boardURL(b.ip),remote:null};row.base=row.base.slice(0,-1);const label=document.createElement('label'),check=document.createElement('input'),status=document.createElement('p');check.type='checkbox';check.disabled=!b.online;label.append(check,document.createTextNode(' '+b.name+' · '+b.ip+(b.self?' · tyto hodiny':'')));status.className='muted';status.textContent=b.online?'Připraveno ke kontrole':'Offline';const block=document.createElement('div');block.className='panel';block.append(label,status);$('fleetTargets').append(block);row.check=check;row.status=status;fleetRows.push(row);}
 $('fleetStatus').textContent=fleetRows.length?'Vyber hodiny. Kompatibilita všech vybraných se ověří před nahráváním.':'Žádné hodiny. Počkej 15 sekund a zkus hledání znovu.';
});
$('fleetStop').onclick=()=>{fleetStopRequested=true;$('fleetStop').disabled=true;$('fleetStatus').textContent='Dokončím aktuální hodiny; další už nespustím.';};
async function fleetVerify(row,oldBoot,hash){for(let i=0;i<20;i++){await delay(3000);let s;try{s=await fleetState(row);}catch(e){continue;}if(s.bootId!==oldBoot){if(s.imageSHA256!==hash)throw Error('Po restartu nesouhlasí otisk firmware.');row.remote=s;return s;}}throw Error('Restart a nový firmware nejsou ověřené. Zkontroluj tyto hodiny.');}
async function fleetOne(row,bytes,hash){
 row.remote=await fleetState(row);if(row.remote.imageSHA256===hash){fleetLine(row,'Tato verze už je nahraná · '+row.remote.firmware);return;}
 const info=await fleetRequest(row,'/api/update/info');if(info.family!=='RUNDEUHR_C3_OTA_LAYOUT_1'||!info.supported||info.busy||bytes.length>info.capacity||!compatibleFirmware(bytes,info.family))throw Error('Zařízení je zaneprázdněné nebo firmware není kompatibilní.');
 const oldBoot=row.remote.bootId;let session='',finishSent=false;
 try{
  let start;for(let i=0;i<20;i++){try{start=await fleetRequest(row,'/api/update/begin',{target:row.id,size:bytes.length,family:info.family},20000);break;}catch(e){if(e.httpStatus!==409||!e.message.includes('Wetterabruf laeuft')||i===19)throw e;fleetLine(row,'Čekám na dokončení načítání dat…');await delay(1000);}}
  if(!start?.session)throw Error('Zařízení nepotvrdilo relaci aktualizace.');session=start.session;
  for(let offset=0;offset<bytes.length;offset+=512){const hex=Array.from(bytes.subarray(offset,offset+512),x=>x.toString(16).padStart(2,'0')).join('');await fleetRequest(row,'/api/update/chunk',{target:row.id,session,offset,hex},20000);fleetLine(row,'Nahrávání '+Math.floor(Math.min(offset+512,bytes.length)*100/bytes.length)+' %');}
  finishSent=true;try{await fleetRequest(row,'/api/update/finish',{target:row.id,session},20000);}catch(e){if(e.httpStatus)throw e;/* Lost acknowledgment: read-only verification, never repeat finish. */}
  fleetLine(row,'Ověřuji restart a otisk firmware…');const s=await fleetVerify(row,oldBoot,hash);fleetLine(row,'Hotovo · '+s.firmware+' · otisk ověřen');
 }catch(e){if(session&&!finishSent)try{await fleetRequest(row,'/api/update/cancel',{target:row.id,session},5000);}catch(_){}throw e;}
}
$('fleetUpload').onclick=safeAction(async()=>{
 if(uploading||demo||!state)return;if(!fleetHostReady())throw Error('Pro hromadnou aktualizaci otevři IP adresu hodin.');const file=$('firmwareFile').files[0];if(!file)throw Error('Vyber soubor .ino.bin nahoře.');const selected=fleetRows.filter(r=>r.check.checked).sort((a,b)=>Number(!!a.self)-Number(!!b.self));if(!selected.length)throw Error('Vyber alespoň jedny hodiny.');const bytes=new Uint8Array(await file.arrayBuffer());if(!compatibleFirmware(bytes,'RUNDEUHR_C3_OTA_LAYOUT_1')||bytes[23]!==1)throw Error('Vyber kompatibilní BIN s ověřovacím otiskem.');const hash=Array.from(bytes.slice(-32),x=>x.toString(16).padStart(2,'0')).join('');
 if(!confirm('Aktualizovat '+selected.length+' zařízení souborem '+file.name+'?\n'+selected.map(r=>r.name+' · '+r.ip).join('\n')))return;
 uploadLock(true);fleetStopRequested=false;let completed=0,failed=false,activeRow=null;selected.forEach(r=>{r.done=false;r.failed=false;});const stop=$('fleetStop');stop.hidden=false;stop.disabled=false;
 try{
  while(inFlight||frameBusy)await delay(50);
  for(const row of selected){if(fleetStopRequested)break;activeRow=row;fleetLine(row,'Kontroluji zařízení…');row.remote=await fleetState(row);const info=await fleetRequest(row,'/api/update/info');if(info.family!=='RUNDEUHR_C3_OTA_LAYOUT_1'||!info.supported||info.busy||bytes.length>info.capacity||!compatibleFirmware(bytes,info.family))throw Error(row.name+': nevyhovující firmware/OTA nebo probíhající aktualizace.');fleetLine(row,'Kontrola prošla');}
  for(const row of selected){if(fleetStopRequested)break;activeRow=row;$('fleetStatus').textContent='Aktualizuji '+(completed+1)+' / '+selected.length+' · '+row.name;try{await fleetOne(row,bytes,hash);completed++;row.done=true;}catch(e){fleetLine(row,'Neověřeno / chyba: '+e.message);throw Error(row.name+': '+e.message);}}
 }catch(e){failed=true;if(activeRow){activeRow.failed=true;fleetLine(activeRow,'Neověřeno / chyba: '+e.message);}$('fleetStatus').textContent='Dávka zastavena: '+e.message;}
 finally{uploadLock(false);stop.hidden=true;if(!failed)$('fleetStatus').textContent=fleetStopRequested?'Zastaveno. Dokončeno '+completed+' / '+selected.length+'.':'Hotovo: '+completed+' / '+selected.length+'.';if(failed||fleetStopRequested)selected.filter(r=>!r.done&&!r.failed).forEach(r=>fleetLine(r,'Neprovedeno · dávka zastavena'));$('fleetUpload').disabled=true;$('fleetRefresh').disabled=true;notify('Před další dávkou obnov stránku a zkontroluj výsledky.');}
});
