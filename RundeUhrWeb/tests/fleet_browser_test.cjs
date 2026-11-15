const {webkit,chromium}=require('playwright');const fs=require('fs'),assert=require('assert');
const api=fs.readFileSync('web/api.js','utf8'),fleet=fs.readFileSync('web/fleet.js','utf8'),ota=fs.readFileSync('web/ota.js','utf8');
const shared=ota.split('\n').filter(l=>l.startsWith('function compatibleFirmware')||l.startsWith('function uploadLock')).join('\n');
const master='192.168.1.10',hosts=[master,'192.168.1.11','192.168.1.12'],ids=['000000000010','000000000011','000000000012'];
const html=`<meta charset="utf-8"><div id="message"></div><div id="fleetOriginHint"></div><button id="fleetRefresh">Najít</button><div id="fleetTargets"></div><input id="firmwareFile" type="file"><button id="fleetUpload">Nahrát</button><button id="fleetStop" hidden>Stop</button><p id="fleetStatus"></p><script>${api}\nlet otaLockedControls=[];${shared}\nconst delay=ms=>new Promise(r=>setTimeout(r,Math.min(ms,10)));state={boardId:'${ids[0]}',ip:'${master}'};online=true;token='token0';${fleet}</script>`;
(async()=>{for(const engine of [webkit,chromium]){const browser=await engine.launch({headless:true});
for(const mode of ['success','lost-finish','chunk-fail','wrong-id','too-large','wrong-hash','already','stop']){
 const page=await browser.newPage();let begins=[],finishes=[],chunks=[],cancels=[],active=null,preflights=0;let errors=[];page.on('pageerror',e=>errors.push(e.message));page.on('dialog',d=>d.accept());let stopped=false;
 const devices=hosts.map((host,i)=>({fleetProtocol:1,boardId:ids[i],deviceLabel:'Hodiny '+i,ip:host,bootId:'old'+i,token:'token'+i,imageSHA256:mode==='already'?'ab'.repeat(32):'cd'.repeat(32),firmware:'3.2.0-rc5'}));
 await page.route('http://192.168.1.*/**',async route=>{const req=route.request(),url=new URL(req.url()),i=hosts.indexOf(url.hostname),p=url.pathname;assert(i>=0);const origin=req.headers().origin;const headers={'access-control-allow-origin':origin||'http://'+master,'access-control-allow-methods':'GET, POST, OPTIONS','access-control-allow-headers':'Content-Type, X-Uhr-Token'};
 if(req.method()==='OPTIONS'){preflights++;await route.fulfill({status:204,headers});return;}
 if(p==='/'){await route.fulfill({contentType:'text/html',body:html});return;}
 let result={ok:true};if(p==='/api/boards')result={boards:hosts.map((ip,j)=>({id:ids[j],name:'Hodiny '+j,ip,self:j===0,online:true}))};
 if(p==='/api/fleet/state')result={...devices[i],boardId:mode==='wrong-id'&&i===2?'ffffffffffff':ids[i]};
 if(p==='/api/update/info')result={supported:true,busy:false,capacity:mode==='too-large'&&i===2?1000:1966080,family:'RUNDEUHR_C3_OTA_LAYOUT_1'};
 if(req.method()==='POST'){const d=req.postDataJSON();assert.equal(req.headers()['x-uhr-token'],devices[i].token);assert.equal(d.target,ids[i]);
 if(p==='/api/update/begin'){assert.equal(active,null,'Only one flash writer');active=i;begins.push(i);result.session='session'+i;}
 if(p==='/api/update/chunk'){assert.equal(active,i);assert.equal(d.session,'session'+i);chunks.push([i,d.offset]);if(mode==='stop'&&!stopped){stopped=true;await page.evaluate(()=>document.getElementById('fleetStop').click());}if(mode==='chunk-fail'){await route.abort('failed');return;}}
 if(p==='/api/update/cancel'){cancels.push(i);active=null;}
 if(p==='/api/update/finish'){finishes.push(i);active=null;devices[i].bootId='new'+i;devices[i].imageSHA256=mode==='wrong-hash'?'ee'.repeat(32):'ab'.repeat(32);devices[i].firmware='3.2.0-rc6';if(mode==='lost-finish'){await route.abort('failed');return;}}
 }
 await route.fulfill({status:200,headers,contentType:'application/json',body:JSON.stringify(result)});
 });
 await page.goto('http://'+master+'/');await page.click('#fleetRefresh');await page.waitForSelector('#fleetTargets input');await page.locator('#fleetTargets input').evaluateAll(els=>els.forEach(e=>e.checked=true));
 const bin=Buffer.alloc(2048);bin[0]=0xe9;bin[12]=5;bin[23]=1;bin.write('RUNDEUHR_C3_OTA_LAYOUT_1',64);bin.fill(0xab,2016);await page.setInputFiles('#firmwareFile',{name:'test.ino.bin',mimeType:'application/octet-stream',buffer:bin});await page.click('#fleetUpload');await page.waitForFunction(()=>document.querySelector('#fleetUpload').disabled&&!uploading);
 assert.deepEqual(errors,[]);if(['success','lost-finish'].includes(mode)){assert.deepEqual(begins,[1,2,0]);assert.deepEqual(finishes,[1,2,0]);assert.equal(chunks.length,12);assert((await page.textContent('#fleetStatus')).includes('Hotovo: 3 / 3'));}
 if(mode==='chunk-fail'){assert.deepEqual(begins,[1]);assert.deepEqual(finishes,[]);assert.deepEqual(cancels,[1]);assert.equal(chunks.length,1);}
 if(['wrong-id','too-large','already'].includes(mode))assert.deepEqual(begins,[]);
 if(mode==='wrong-hash'){assert.deepEqual(begins,[1]);assert((await page.textContent('#fleetStatus')).includes('nesouhlasí'));}
 if(mode==='stop'){assert.deepEqual(begins,[1]);assert.deepEqual(finishes,[1]);assert((await page.textContent('#fleetStatus')).includes('Zastaveno'));}
 console.log('PASS',engine.name(),mode,'sequential fleet, identity/size/hash, no blind write retry, explicit status');await page.close();
}await browser.close();}})().catch(e=>{console.error(e);process.exit(1)});
