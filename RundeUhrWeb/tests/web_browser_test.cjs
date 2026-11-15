const {webkit,chromium}=require('playwright');
const fs=require('fs'),assert=require('assert');
const html=fs.readFileSync('preview/App.html','utf8');
const base={schema:4,deviceName:'Kuchyň',deviceLabel:'RundeUhr Kuchyň',hostname:'rundeuhr-kuchyn-abcd',setupSSID:'RundeUhr-Kuchyn-ABCD',firmware:'3.0.0-rc1',model:'ESP32-C3',flash:4194304,sketchSize:1503632,heap:85000,minHeap:62000,boardId:'0123456789ab',bootId:'bootA',token:'tokenA',view:0,mask:1023,interval:15,automatic:false,animations:true,city:'Bern',lat:46.948,lon:7.4474,event:'Ferien',date:'',countdownDates:['2027-08-01','2027-01-01','','','2027-03-01'],customEvent:'Výlet',countdownType:0,weekendStartDay:5,weekendEndDay:1,weekendStartMinute:960,weekendEndMinute:0,wifi:true,ssid:'Domov',ip:'192.168.1.42',rssi:-51,timeValid:true,weatherAge:30,weatherAt:1794726000,weatherBusy:false};
function bmp(){const b=Buffer.alloc(28918);b.write('BM');b.writeUInt32LE(b.length,2);b.writeUInt32LE(118,10);b.writeUInt32LE(40,14);b.writeInt32LE(240,18);b.writeInt32LE(240,22);b.writeUInt16LE(1,26);b.writeUInt16LE(4,28);b.writeUInt32LE(28800,34);b.writeUInt32LE(16,46);return b;}
(async()=>{
 for(const [engine,launch] of (process.env.WEBKIT_ONLY?[['webkit',webkit]]:process.env.CHROMIUM_ONLY?[['chromium',chromium]]:[['webkit',webkit],['chromium',chromium]])){
 const browser=await launch.launch({headless:true,...(engine==='chromium'&&process.env.SYSTEM_CHROMIUM?{executablePath:'/usr/bin/chromium',args:['--no-sandbox']}:{} )});const page=await browser.newPage({viewport:{width:1440,height:1024}});let state=structuredClone(base),calls=[],fail=false,otaMode='ok',errors=[],postCount=0;
 page.on('pageerror',e=>errors.push(e.message));page.on('dialog',d=>d.accept());
 await page.route('http://rundeuhr.local/**',async route=>{
 const r=route.request(),p=new URL(r.url()).pathname,data=r.method()==='POST'?r.postDataJSON():null;
 if(p==='/'){await route.fulfill({contentType:'text/html',body:html});return;}
 if(fail){await route.abort('failed');return;}
 calls.push({p,data});if(data){postCount++;assert.equal(r.headers()['x-uhr-token'],'tokenA');}
 if(p==='/frame.bmp'){await route.fulfill({contentType:'image/bmp',body:bmp()});return;}
 let result={ok:true,message:'Uloženo'};
 if(p==='/api/state')result=state;
 if(p==='/api/settings')Object.assign(state,data);
 if(p==='/api/view')state.view=data.view;
 if(p==='/api/boards')result={boards:[{name:'Kuchyň',self:true,online:true},{name:'Ložnice',online:true,ip:'192.168.1.43'}]};
 if(p==='/api/wifis')result={networks:[{ssid:'Domov',connected:true}]};
 if(p==='/api/scan'&&!data)result={running:false,networks:[{ssid:'<img src=x onerror=alert(1)>',rssi:-40,secure:true},{ssid:'Druhá síť',rssi:-60,secure:true}]};
 if(p==='/api/update/info')result={supported:true,capacity:1966080,family:'RUNDEUHR_C3_OTA_LAYOUT_1'};
 if(p==='/api/update/begin')result={session:'sessionA',ok:true};
 if(p==='/api/update/chunk'&&otaMode==='lost-chunk'){await route.abort('failed');return;}
 if(p==='/api/update/finish'){state.bootId='bootB';state.imageSHA256='ab'.repeat(32);if(otaMode==='lost-finish'){await route.abort('failed');return;}}
 if(p==='/api/config/export')result={...state,backupFamily:'RundeUhr-settings-v1'};
 await route.fulfill({contentType:'application/json',body:JSON.stringify(result)});
 });
 await page.goto('http://rundeuhr.local/');await page.waitForFunction(()=>document.getElementById('connection').textContent==='Zařízení připojeno');
 await page.waitForFunction(()=>document.getElementById('previewStatus').textContent.includes('Skutečný snímek'));
 assert.equal(await page.locator('.screen-card').count(),10);
 await page.locator('[data-goto="screens"]').first().click();
 for(let i=0;i<10;i++){await page.locator('#card'+i+' .tile').click();await page.waitForFunction(i=>document.querySelector('#card'+i+' .tile').getAttribute('aria-pressed')==='true',i);}
 await page.locator('#cycle4').uncheck();await page.waitForFunction(()=>!(state.mask&16));assert.equal(state.mask&16,0);await page.locator('#card4 .tile').click();assert.equal(state.view,4);
 await page.evaluate(()=>{state.mask=1;paint();});state.mask=1;await page.locator('#cycle0').click();await page.waitForTimeout(100);assert(await page.locator('#cycle0').isChecked());assert((await page.locator('#message').innerText()).includes('alespoň'));
 await page.evaluate(()=>{state.mask=1023;paint();});state.mask=1023;
 await page.locator('nav a[href="#controls"]').click();await page.locator('#automatic').check();await page.locator('#interval').fill('27');await page.locator('#cycleForm button').click();await page.waitForFunction(()=>state.interval===27);assert(state.automatic);assert.equal(state.interval,27);
 await page.locator('nav a[href="#countdowns"]').click();await page.locator('#countdownType').selectOption('4');await page.locator('#event').fill('Moje událost');await page.locator('#date').fill('2027-05-09');await page.locator('#countdownForm button').click();await page.waitForTimeout(150);assert.equal(state.customEvent,'Moje událost');assert.equal(state.countdownDates[0],'2027-08-01');assert.equal(state.countdownDates[4],'2027-05-09');
 await page.locator('#weekendStartTime').fill('15:30');await page.locator('#weekendForm button').click();await page.waitForTimeout(150);assert.equal(state.weekendStartMinute,930);
 await page.locator('nav a[href="#wifi"]').click();await page.waitForSelector('#networks button');assert.equal(await page.locator('#networks img').count(),0);await page.locator('#networks button').nth(1).click();assert.equal(await page.locator('#ssid').inputValue(),'Druhá síť');await page.locator('#pass').fill('abcdefgh123');await page.locator('#showPass').check();assert.equal(await page.locator('#pass').getAttribute('type'),'text');await page.locator('#wifiForm button').click();await page.waitForTimeout(150);assert.equal(await page.locator('#pass').inputValue(),'');assert(calls.some(c=>c.p==='/api/wifi'&&c.data.pass==='abcdefgh123'));
 await page.locator('nav a[href="#system"]').click();await page.locator('#city').fill('Rozepsaná úprava');await page.evaluate(()=>poll());assert.equal(await page.locator('#city').inputValue(),'Rozepsaná úprava');
 fail=true;await page.evaluate(()=>poll());assert(!(await page.locator('#offline').isHidden()));fail=false;await page.evaluate(()=>poll());assert(await page.locator('#offline').isHidden());
 await page.locator('nav a[href="#extensions"]').click();assert.equal(await page.locator('.module-card').count(),7);await page.locator('.module-card').nth(3).click();assert((await page.locator('#moduleTitle').innerText()).includes('Bambu'));assert(await page.locator('#moduleDetail input').isDisabled());
 await page.locator('nav a[href="#overview"]').click();await page.screenshot({path:'tests/'+engine+'-desktop.png',fullPage:true});
 await page.setViewportSize({width:390,height:844});
 for(const id of ['overview','screens','controls','appearance','countdowns','wifi','extensions','system']){await page.evaluate(id=>go(id),id);await page.waitForTimeout(80);const widths=await page.evaluate(()=>[document.documentElement.scrollWidth,innerWidth]);assert(widths[0]<=widths[1],id+' horizontal overflow: '+widths);if(id==='screens'){const box=await page.locator('#card0 img').boundingBox();assert(Math.abs(box.width-box.height)<1,'Preview must stay circular');}if(engine==='webkit')await page.screenshot({path:'tests/webkit-mobile-'+id+'.png',fullPage:true});}
 // Reject wrong-chip/oversized images before beginning OTA.
 await page.evaluate(()=>go('system'));await page.locator('#firmwareFile').setInputFiles({name:'wrong.ino.bin',mimeType:'application/octet-stream',buffer:Buffer.alloc(2048)});await page.locator('#uploadFirmware').click();await page.waitForTimeout(120);assert(!calls.some(c=>c.p==='/api/update/begin'));
 const fw=Buffer.alloc(2048);fw[0]=0xe9;fw[12]=5;fw[23]=1;fw.write('RUNDEUHR_C3_OTA_LAYOUT_1',64);fw.fill(0xab,fw.length-32);
 otaMode='lost-chunk';await page.locator('#firmwareFile').setInputFiles({name:'valid.ino.bin',mimeType:'application/octet-stream',buffer:fw});await page.locator('#uploadFirmware').click();await page.waitForFunction(()=>!uploading);assert((await page.locator('#otaStatus').innerText()).includes('nebyla dokončena'));assert(calls.some(c=>c.p==='/api/update/cancel'),JSON.stringify(calls.slice(-10))); assert(!calls.some(c=>c.p==='/api/update/finish'));
 otaMode='lost-finish';await page.locator('#uploadFirmware').click();await page.waitForFunction(()=>document.getElementById('otaStatus').textContent.includes('Ověřen restart'),null,{timeout:15000});assert(await page.locator('#uploadFirmware').isDisabled());assert.equal(errors.length,0,errors.join('\n'));
 console.log('PASS '+engine+': real browser + simulated ESP API, 10 selections, cycle switches/manual selection, empty-cycle protection, settings/date preservation, weekend edit, Wi-Fi/XSS/password clearing, offline recovery, modules, all 390px layouts, 28918-byte BMP, invalid firmware, interrupted chunks, lost finish acknowledgment + SHA verification.');
 await browser.close();
 }
})().catch(e=>{console.error(e);process.exit(1);});
