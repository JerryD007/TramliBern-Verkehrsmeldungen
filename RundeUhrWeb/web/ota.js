// Keep the compatible chunk protocol. Never retry a flash write blindly.
let otaInfo=null,otaLockedControls=[];
async function loadOtaInfo(){try{otaInfo=demo?{supported:false,capacity:null}:await request('/api/update/info');$('otaInfo').textContent=otaInfo.supported?'Kapacita jednoho OTA oddílu: '+size(otaInfo.capacity):demo?'OTA v ukázce není dostupné.':'Chybí vhodný OTA oddíl. Je nutná instalace přes USB s odpovídající tabulkou oddílů.';$('uploadFirmware').disabled=!otaInfo.supported||uploading;}catch(e){$('otaInfo').textContent='Kapacitu OTA nelze ověřit.';$('uploadFirmware').disabled=true;}}
function uploadLock(on){uploading=on;document.body.classList.toggle('uploading',on);if(on){clearTimeout(scanTimer);otaLockedControls=[...document.querySelectorAll('button,input,select')].map(el=>[el,el.disabled]);otaLockedControls.forEach(([el])=>el.disabled=true);}else{otaLockedControls.forEach(([el,disabled])=>el.disabled=disabled);otaLockedControls=[];}}
function compatibleFirmware(bytes,family){if(bytes.length<1024||bytes[0]!==0xe9||bytes[12]!==5||bytes[13]!==0)return false;const marker=new TextEncoder().encode(family);outer:for(let i=0;i<=bytes.length-marker.length;i++){for(let j=0;j<marker.length;j++)if(bytes[i+j]!==marker[j])continue outer;return true;}return false;}
const delay=ms=>new Promise(resolve=>setTimeout(resolve,ms));
async function verifyReboot(target,oldBoot,expectedHash){for(let attempt=0;attempt<20;attempt++){await delay(3000);try{const s=await request('/api/state',{},4000);if(s.boardId!==target)throw Error('Odpovídá jiné zařízení.');if(s.bootId!==oldBoot){if(expectedHash&&s.imageSHA256===expectedHash){$('otaStatus').textContent='Aktualizace dokončena. Ověřen restart i otisk nového firmware '+s.firmware+'. Obnov stránku.';return;}if(expectedHash&&s.imageSHA256){$('otaStatus').textContent='Zařízení se restartovalo, ale otisk firmware neodpovídá nahrané verzi. Zkontroluj diagnostiku.';return;}}}catch(e){/* Device may be rebooting. Do not claim success. */}}$('otaStatus').textContent='Restart a novou verzi se nepodařilo ověřit. Znovu otevři místní adresu hodin a zkontroluj verzi. Nenahrávej soubor opakovaně bez kontroly.';}
$('uploadFirmware').onclick=safeAction(async()=>{
 if(demo){notify('Aktualizace je dostupná jen na skutečných hodinách.');return;}
 if(uploading||!state)return;const f=$('firmwareFile').files[0];if(!f)throw Error('Nejdříve vyber .ino.bin.');
 let session='',finishSent=false,accepted=false,target=state.boardId,oldBoot=state.bootId,expectedHash='';
 uploadLock(true);
 try{
  // Drain requests already in flight before starting flash writes.
  while(inFlight||frameBusy)await delay(50);
  const info=await request('/api/update/info');if(!info.supported||f.size>info.capacity||f.size<1024)throw Error('Soubor se nevejde do OTA oddílu nebo není platný firmware.');
  const bytes=new Uint8Array(await f.arrayBuffer());if(!compatibleFirmware(bytes,info.family))throw Error('Soubor není kompatibilní firmware RundeUhr pro ESP32-C3.');
  if(bytes[23]===1)expectedHash=Array.from(bytes.slice(-32),b=>b.toString(16).padStart(2,'0')).join('');
  if(!confirm('Nahrát '+f.name+' do '+state.deviceLabel+' ('+state.ip+')?'))return;
  $('otaProgress').value=0;$('otaStatus').textContent='Připravuji aktualizaci…';
  const start=await post('/api/update/begin',{target,size:bytes.length,family:info.family},20000);session=start.session;
  for(let offset=0;offset<bytes.length;offset+=512){const hex=Array.from(bytes.subarray(offset,offset+512),v=>v.toString(16).padStart(2,'0')).join('');await post('/api/update/chunk',{target,session,offset,hex},20000);const pct=Math.floor(Math.min(offset+512,bytes.length)*100/bytes.length);$('otaProgress').value=pct;$('otaStatus').textContent='Nahrávání: '+pct+' % · ponech Safari otevřené';}
  finishSent=true;await post('/api/update/finish',{target,session},20000);accepted=true;session='';$('otaStatus').textContent='Firmware přijat. Čekám na restart a ověření nové verze…';await verifyReboot(target,oldBoot,expectedHash);
 }catch(e){if(finishSent){$('otaStatus').textContent='Potvrzení dokončení není jisté: '+e.message+' Ověřuji zařízení…';await verifyReboot(target,oldBoot,expectedHash);}else{$('otaStatus').textContent='Aktualizace nebyla dokončena: '+e.message;if(session)try{await post('/api/update/cancel',{target,session},5000);}catch(_){} }}
 finally{uploadLock(false);if(finishSent){$('uploadFirmware').disabled=true;notify('Před další aktualizací obnov stránku a zkontroluj verzi firmware.');}}
});
window.addEventListener('beforeunload',e=>{if(uploading){e.preventDefault();e.returnValue='Probíhá aktualizace';}});
const initialHash=location.hash.slice(1).split('/');go(initialHash[0]||'overview');if(initialHash[0]==='extensions'&&initialHash[1])openModule(initialHash[1]);
setInterval(poll,5000);document.addEventListener('visibilitychange',()=>{if(!document.hidden)poll();});
poll().then(()=>{loadOtaInfo();if(activePage==='wifi'){loadNetworks();scan();}});
