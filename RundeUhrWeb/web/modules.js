// Registry only. No network clients, background jobs or screen replacements.
const modules=[
 {id:'currencies',name:'Kurzy světových měn',description:'Budoucí výběr měn a základní měny.'},
 {id:'crypto',name:'Kryptoměny',description:'Budoucí výběr kryptoměn a zobrazované měny.'},
 {id:'aircraft',name:'Sledování letadel',description:'Budoucí konfigurace zdroje dat, polohy a oblasti sledování.'},
 {id:'bambu',name:'Bambu Lab X1C',description:'Budoucí přehled tiskárny a průběhu tisku.'},
 {id:'astronomy',name:'Rozšířená astronomie',description:'Budoucí výběr dalších astronomických informací.'},
 {id:'homeassistant',name:'Home Assistant',description:'Budoucí propojení s místní instalací Home Assistant.'},
 {id:'future',name:'Další moduly',description:'Místo pro další postupně doplňované funkce.'}
];
function openModule(id){const m=modules.find(m=>m.id===id);if(!m)return;go('extensions');$('moduleList').hidden=true;$('moduleDetail').hidden=false;$('moduleTitle').textContent=m.name;$('moduleDescription').textContent=m.description;history.replaceState(null,'','#extensions/'+m.id);}
modules.forEach(m=>{const b=document.createElement('button');b.className='module-card';const left=document.createElement('span'),title=document.createElement('strong'),status=document.createElement('span'),arrow=document.createElement('span');title.textContent=m.name;status.className='badge';status.textContent='Připravuje se';arrow.className='arrow';arrow.textContent='›';left.append(title,status);b.append(left,arrow);b.onclick=()=>openModule(m.id);$('moduleList').append(b);});
$('closeModule').onclick=()=>{go('extensions');};
