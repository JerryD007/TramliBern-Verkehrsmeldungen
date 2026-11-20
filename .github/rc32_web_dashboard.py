from pathlib import Path
import sys
p=Path(sys.argv[1])
s=p.read_text(encoding="utf-8")
assert 'constexpr const char *VERSION = "TramliBern 4.0 RC31 HomeCross";' in s
s=s.replace('constexpr const char *VERSION = "TramliBern 4.0 RC31 HomeCross";','constexpr const char *VERSION = "TramliBern 4.0 RC32 HomeCross";',1)
s=s.replace('Version 4.0 RC31 HomeCross · 3 Seiten','Version 4.0 RC32 HomeCross · 3 Seiten',1)
oldcss='input[type=checkbox]{display:inline-block;width:24px;min-height:24px;height:24px;margin:0 12px 0 0;vertical-align:middle}</style>'
assert s.count(oldcss)==1
css=r"""
/* RC32 HomeCross WEB ONLY: dark, high-contrast, responsive dashboard */
:root{color-scheme:dark;--ink:#f4faff;--muted:#b7c8da;--blue:#00c9ce;--teal:#00d2c9;--line:#31455c}
body{background:#07111f;color:var(--ink);font-size:17px;line-height:1.48}
header{background:#0b1728;border-bottom:1px solid #26465a;padding:17px 24px}
header div,main{max-width:1440px}
header b{font-size:24px;color:#fff}
header span{font-size:12px;letter-spacing:1px;color:#78e8e2}
main{padding:18px 22px 44px}
h1{font-size:clamp(23px,3vw,32px);letter-spacing:-.35px}
h2{font-size:21px}
p{margin:10px 0 16px}
.card{background:#111f33;border:1px solid #2b4059;border-top:1px solid #2b4059;box-shadow:none;border-radius:15px;padding:20px;margin:15px 0}
.card:has(form[action='/city']),.card:has(form[action='/search']),.card:has(button[aria-current='page']){background:#111f33;border-color:#31455c}
small,.muted,.hint,.address{color:#b7c8da}
small{font-size:14px}.hint{font-size:14px}
.card h2{font-size:20px}
a{color:#35e2dc}
button,.button{font-size:16px;min-height:44px;border-radius:10px;padding:11px 15px;background:#00c5c3;color:#04121a;font-weight:750}
button:hover,.button:hover{filter:brightness(1.08)}
button:focus-visible,a:focus-visible,input:focus-visible,select:focus-visible{outline:3px solid #ffce37;outline-offset:3px}
input,select{font-size:17px;min-height:45px;background:#0c1b2d;color:#f4faff;border:1px solid #526780;border-radius:9px}
input:focus,select:focus{background:#0c2131;border-color:#00d2c9}
input::placeholder{color:#acbfd0}label{font-size:16px}
.subtle{background:#1a3048;color:#f1faff;border:1px solid #526780}
nav a{background:#15263b;color:#f0fbff;border-color:#334961;font-size:15px;min-height:42px}
nav a:hover{background:#1b3548}
main>nav:not(.hubnav){display:none}
.hubnav{display:flex;gap:8px;margin:12px 0 20px;flex-wrap:wrap}
.hubnav a{flex:0 1 auto;padding:9px 12px;min-height:42px;white-space:normal}
.hubnav a[aria-current=page]{background:#005f66;border-color:#00cfc8}
.status{background:#0d433d;color:#98ffcf;border:1px solid #168b7a;font-size:16px}
.notice{background:#3a2d13;color:#ffdf8d;border-left:4px solid #ffbe33}
footer{color:#a9bed3;border-color:#2b4158}
.hubgrid{display:grid;grid-template-columns:minmax(230px,300px) minmax(0,1fr);gap:14px;align-items:start}
.hubrail,.hubpanel{background:#0e1c2e;border:1px solid #2b415b;border-radius:16px;padding:15px;min-width:0}
.hubrail h2{margin:2px 0 13px}
.devicecard{display:block;text-decoration:none;color:#f6fcff;background:#14243b;border:1px solid #354b64;border-radius:12px;padding:12px;margin-bottom:10px;min-width:0}
.devicecard.sel{background:#073f44;border:2px solid #00dad1;padding:11px}
.devicecard b{display:block;font-size:18px;overflow-wrap:anywhere}
.devicecard small{display:block;font-size:13px;overflow-wrap:anywhere}
.devicecard .dot{display:inline-block;width:9px;height:9px;border-radius:50%;background:#7f95ab;margin-right:6px}
.devicecard.sel .dot{background:#0ee5ab}
.hubtools{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:10px}
.hubtools a,.hubtools button{display:flex;align-items:center;justify-content:center;background:#172b43;color:#f5ffff;border:1px solid #3c566d;padding:14px 6px;border-radius:12px;text-decoration:none;min-height:68px;font-size:15px;text-align:center}
.hubtools form{display:flex;margin:0}.hubtools form button{width:100%}
.hubtools a:hover{border-color:#00d0c9;background:#103b49}
.hubhead{display:flex;flex-wrap:wrap;justify-content:space-between;gap:9px;align-items:center;margin-bottom:16px}
.hubtag{display:inline-block;background:#0b4c43;color:#7af8c8;border-radius:30px;padding:5px 11px;font-size:13px}
.hubstats{display:grid;grid-template-columns:1fr 1fr;gap:11px;margin:12px 0}
.hubstat{border:1px solid #344a61;border-radius:12px;padding:14px;background:#13253a;overflow-wrap:anywhere}
.hubstat strong{display:block;font-size:18px}
.hubstat small{color:#b7c8da}
.hubiframe{border:1px solid #40536a;width:100%;height:710px;border-radius:12px;background:#0e1b2c}
.hubnote{color:#b7c8da;font-size:14px}
.hubsection{margin-top:17px}
.hubactions{display:flex;gap:8px;flex-wrap:wrap}
.hubactions>a{flex:1 1 150px}
@media(max-width:850px){.hubgrid{grid-template-columns:1fr}.hubrail{order:0}.hubpanel{order:1}.hubtools{grid-template-columns:repeat(2,minmax(0,1fr))}}
@media(max-width:520px){body{font-size:16px}header{padding:14px}main{padding:13px}h1{font-size:26px}.card{padding:14px}.hubrail,.hubpanel{padding:12px}.hubnav{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:7px}.hubnav a{font-size:14px;justify-content:center;min-height:41px;text-align:center}.hubtools{grid-template-columns:repeat(2,minmax(0,1fr))}.hubstats{grid-template-columns:1fr}.hubiframe{height:620px}.hubactions a{width:100%}button,.button{width:100%}}
"""
s=s.replace(oldcss,oldcss[:-len('</style>')]+css+'</style>',1)
old='  html += "<h1>" + escapeHtml(title) + "</h1><p class=\'muted\'>" + escapeHtml(boardLabel()) + " · " + escapeHtml(currentAddress()) + "</p>";'
assert s.count(old)==1
new='''  html += "<nav class='hubnav' aria-label='Hlavní navigace'><a href='/'>⌂ Přehled</a><a href='/boards'>▣ Displeje</a><a href='/stops'>▤ Zastávky</a><a href='/wifi'>◉ Wi-Fi</a><a href='/display'>☼ Displej</a><a href='/ota'>↥ Aktualizace</a><a href='/comfort'>⚙ Pokročilé</a></nav>";
  html += "<h1>" + escapeHtml(title) + "</h1><p class='muted'>" + escapeHtml(boardLabel()) + " · " + escapeHtml(currentAddress()) + "</p>";'''
s=s.replace(old,new,1)

# The selected remote board is operated inside an iframe. This does NOT require CORS;
# browser policies or devices may prevent embedding: an explicit open link is always available.
a=s.index('  server.on("/boards", HTTP_GET, []() {')
b=s.index('  server.on("/boards/add"',a)
boards=r'''  server.on("/boards", HTTP_GET, []() {
    String h=pageStart("Moje displeje");
    int n=prefs.getUChar("peerCount",0); if(n>12)n=0;
    h+="<section class='card'><h2>Spravovat až 13 zařízení</h2><p class='hubnote'>Tento seznam je uložený na aktuálním ESP32. Další zařízení se přidávají jejich místní IP adresou nebo hostname. Každý displej má vlastní nastavení a OTA.</p></section>";
    h+="<div class='hubgrid'><section class='hubrail'><h2>Uložené displeje</h2>";
    h+="<a class='devicecard sel' href='/'><b><span class='dot'></span>"+escapeHtml(boardLabel())+"</b><small>Právě otevřený · "+escapeHtml(currentAddress())+"</small></a>";
    for(int i=0;i<n;i++){
      String address=normalizeBoardAddress(prefs.getString(("peerA"+String(i)).c_str(),""));
      String name=prefs.getString(("peerN"+String(i)).c_str(),"Board");
      h+="<div class='devicecard'><b><span class='dot'></span>"+escapeHtml(name)+"</b><small>"+escapeHtml(address)+" · Stav neověřen</small>";
      if(address.length())h+="<p><a class='button full' href='"+escapeHtml(address)+"'>Otevřít displej</a></p>";
      h+="<form method='post' action='/boards/remove'><input type='hidden' name='index' value='"+String(i)+"'><button class='subtle full'>Odebrat ze seznamu</button></form></div>";
    }
    h+="</section><section class='hubpanel'><h2>+ Přidat displej</h2>";
    if(n<12)h+="<form method='post' action='/boards/add'><label for='peerName'>Název zařízení</label><input id='peerName' name='name' maxlength='60' placeholder='Například Kuchyně' required><label for='peerAddress'>IP adresa nebo místní název</label><input id='peerAddress' name='address' maxlength='100' placeholder='192.168.1.43' autocapitalize='none' spellcheck='false' required><button class='full'>Přidat displej</button></form>";
    else h+="<p>Seznam již obsahuje maximální počet zařízení.</p>";
    h+="<p class='hubnote'>Při změně domácí Wi-Fi se může změnit IP adresa. Pro stabilní přístup nastav v routeru rezervaci IP. Vzdálené displeje nelze bez ověření označit jako online.</p>";
    h+="<div class='hubactions'><a class='button subtle' href='/'>Zpět na přehled</a><a class='button subtle' href='/settings'>Název tohoto displeje</a></div></section></div>";
    sendPage(h);
  });
'''
s=s[:a]+boards+s[b:]

a=s.index('  server.on("/",HTTP_GET,[](){')
b=s.index('  setupComfortRoutes();',a)
home=r'''  server.on("/",HTTP_GET,[](){
    String h=pageStart("Správa displejů");
    int n=prefs.getUChar("peerCount",0);if(n>12)n=0;
    int chosen=-1;
    if(server.hasArg("peer")){
      String v=server.arg("peer");bool ok=v.length()>0&&v.length()<3;
      for(size_t i=0;i<v.length();++i)if(v[i]<'0'||v[i]>'9')ok=false;
      if(ok){int index=v.toInt();if(index>=0&&index<n)chosen=index;}
    }
    h+="<div class='hubgrid'><aside class='hubrail'><div class='hubhead'><h2>Moje displeje</h2><span class='hubtag'>"+String(n+1)+" uložených</span></div>";
    h+="<a class='devicecard "+String(chosen<0?"sel":"")+"' href='/'><b><span class='dot'></span>"+escapeHtml(boardLabel())+"</b><small>Aktuální ESP32 · "+escapeHtml(currentAddress())+"</small></a>";
    for(int i=0;i<n;i++){
      String name=prefs.getString(("peerN"+String(i)).c_str(),"Board");
      String addr=normalizeBoardAddress(prefs.getString(("peerA"+String(i)).c_str(),""));
      h+="<a class='devicecard "+String(chosen==i?"sel":"")+"' href='/?peer="+String(i)+"'><b><span class='dot'></span>"+escapeHtml(name)+"</b><small>"+escapeHtml(addr)+" · Stav neověřen</small></a>";
    }
    h+="<a class='button full' href='/boards'>+ Přidat / spravovat displeje</a>";
    h+="<p class='hubnote'>Výběrem dalšího displeje se otevře jeho lokální webové ovládání. Všechny změny platí pouze pro vybraný displej.</p></aside>";
    h+="<section class='hubpanel'>";
    if(chosen>=0){
      String name=prefs.getString(("peerN"+String(chosen)).c_str(),"Board");
      String address=normalizeBoardAddress(prefs.getString(("peerA"+String(chosen)).c_str(),""));
      h+="<div class='hubhead'><div><h2>"+escapeHtml(name)+"</h2><small>"+escapeHtml(address)+"</small></div><span class='hubtag'>Stav neověřen</span></div>";
      h+="<p class='hubnote'>Ovládání dalšího zařízení se zobrazí níže. Pokud Safari vloženou stránku nepovolí, otevři ji samostatně.</p>";
      if(address.length()){
        h+="<div class='hubactions'><a class='button' href='"+escapeHtml(address)+"'>Otevřít celé ovládání</a><a class='button subtle' href='/'>Zpět na toto ESP32</a></div>";
        h+="<iframe class='hubiframe' title='Ovládání "+escapeHtml(name)+"' loading='lazy' src='"+escapeHtml(address)+"'></iframe>";
      }
    }else{
      h+="<div class='hubhead'><div><h2>"+escapeHtml(boardLabel())+"</h2><small>TramliBern HomeCross · "+escapeHtml(currentAddress())+"</small></div><span class='hubtag'>"+String(WiFi.status()==WL_CONNECTED?"Wi-Fi připojena":"Nastavovací režim")+"</span></div>";
      h+="<div class='hubstats'><div class='hubstat'><small>Firmware</small><strong>"+escapeHtml(String(VERSION))+"</strong></div><div class='hubstat'><small>Aktivní město a stránka</small><strong>"+escapeHtml(String(CITY_QUERIES[activeCity]))+" · "+String(activePage+1)+"/3</strong></div></div>";
      h+="<h2 class='hubsection'>Rychlé nastavení</h2><div class='hubtools'>";
      h+="<a href='/stops'>▤ Zastávky / linky</a><a href='/wifi'>◉ Wi-Fi</a><a href='/display'>☼ Jas a čas</a>";
      h+="<form action='/screen-off' method='post'><button type='submit'>☾ Zhasnout</button></form>";
      h+="<form action='/screen-on' method='post'><button type='submit'>☀ Probudit</button></form>";
      h+="<a href='/ota'>↥ Aktualizace OTA</a><a href='/cities'>↔ Město</a><a href='/comfort'>⚙ Pokročilé</a><a href='/diagnostics'>ⓘ Diagnostika</a></div>";
      h+="<section class='card'><h2>Aktualizace firmwaru</h2><p class='hubnote'>Nahrávání .ino.bin s procenty a průběhovým pruhem. Po dokončení ověř verzi na displeji.</p><a class='button full' href='/ota'>Otevřít aktualizaci OTA</a></section>";
      h+="<div class='hubactions'><form method='post' action='/refresh'><button>↻ Obnovit odjezdy</button></form><a class='button subtle' href='/boards'>Správa dalších displejů</a><a class='button subtle' href='/battery'>Baterie</a><a class='button subtle' href='/system'>Systém</a></div>";
    }
    h+="</section></div>";
    sendPage(h);
  });
'''
s=s[:a]+home+s[b:]
assert 'void drawBoard()' in s
assert 'otaFromHomeWifi()' in s
assert 'showWifiQR()' in s
assert 'req.upload.onprogress' in s
assert 'setScreenSleep(' in s
assert 'prefs.getUChar("peerCount"' in s
p.write_text(s,encoding="utf-8")
print("RC32 UI-only patch successful; original firmware backend preserved")
