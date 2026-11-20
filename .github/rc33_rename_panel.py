from pathlib import Path
import sys
p=Path(sys.argv[1]);s=p.read_text(encoding="utf-8")
def once(a,b):
 global s
 assert s.count(a)==1,("RC33 target mismatch",s.count(a),a[:100])
 s=s.replace(a,b,1)
once('constexpr const char *VERSION = "TramliBern 4.0 RC32 HomeCross";','constexpr const char *VERSION = "TramliBern 4.0 RC33 HomeCross";')
once('Version 4.0 RC32 HomeCross · 3 Seiten','Version 4.0 RC33 HomeCross · 3 Seiten')
# Rename the local board using the existing /device handler; rename stored peer nicknames separately.
once('''      h+="<div class='devicecard'><b><span class='dot'></span>"+escapeHtml(name)+"</b><small>"+escapeHtml(address)+" · Stav neověřen</small>";''','''      h+="<div class='devicecard'><b><span class='dot'></span>"+escapeHtml(name)+"</b><small>"+escapeHtml(address)+" · Stav neověřen</small>";
      h+="<form method='post' action='/boards/rename'><input type='hidden' name='index' value='"+String(i)+"'><label for='rename"+String(i)+"'>Jméno v tomto seznamu</label><input id='rename"+String(i)+"' name='name' maxlength='60' value='"+escapeHtml(name)+"' required><button type='submit'>Přejmenovat</button></form>";''')
once('''    h+="<div class='hubactions'><a class='button subtle' href='/'>Zpět na přehled</a><a class='button subtle' href='/settings'>Název tohoto displeje</a></div></section></div>";''','''    h+="<section class='card'><h2>Přejmenovat tento displej</h2><p class='hubnote'>Tento název ukládá přímo aktuální ESP32.</p><form method='post' action='/device'><label for='localBoardName'>Jméno aktuálního ESP32</label><input id='localBoardName' name='name' maxlength='60' required value='"+escapeHtml(boardLabel())+"'><button>Uložit jméno</button></form></section>";
    h+="<div class='hubactions'><a class='button subtle' href='/'>Zpět na přehled</a><a class='button subtle' href='/settings'>Další nastavení</a></div></section></div>";''')
once('''  server.on("/boards/remove", HTTP_POST, []() {''','''  server.on("/boards/rename", HTTP_POST, []() {
    int count=prefs.getUChar("peerCount",0),index;
    if(count>12||!readIndex(count,index)){showMessage("Neplatný displej.",400);return;}
    String name=server.arg("name");name.trim();
    if(!name.length()||name.length()>60){showMessage("Jméno musí mít 1 až 60 znaků.",400);return;}
    prefs.putString(("peerN"+String(index)).c_str(),name);
    server.sendHeader("Location","/?peer="+String(index));server.send(303);
  });
  server.on("/boards/remove", HTTP_POST, []() {''')
# A single navigation/list. Never nest a second board dashboard in an iframe.
once('''      h+="<p class='hubnote'>Ovládání dalšího zařízení se zobrazí níže. Pokud Safari vloženou stránku nepovolí, otevři ji samostatně.</p>";
      if(address.length()){
        h+="<div class='hubactions'><a class='button' href='"+escapeHtml(address)+"'>Otevřít celé ovládání</a><a class='button subtle' href='/'>Zpět na toto ESP32</a></div>";
        h+="<iframe class='hubiframe' title='Ovládání "+escapeHtml(name)+"' loading='lazy' src='"+escapeHtml(address)+"'></iframe>";
      }''','''      h+="<p class='hubnote'>Vpravo jsou přímé odkazy na ovládání vybraného zařízení. Seznam displejů se již neopakuje. Vzdálený stav není automaticky ověřen.</p>";
      if(address.length()){
        h+="<div class='hubtools'><a href='"+escapeHtml(address)+"/stops'>▤ Zastávky / linky</a><a href='"+escapeHtml(address)+"/wifi'>◉ Wi-Fi</a><a href='"+escapeHtml(address)+"/display'>☼ Jas a čas</a><a href='"+escapeHtml(address)+"/ota'>↥ Aktualizace OTA</a><a href='"+escapeHtml(address)+"/settings'>⚙ Nastavení</a><a href='"+escapeHtml(address)+"'>⌂ Celé ovládání</a></div>";
        h+="<section class='card'><h2>Přejmenovat v seznamu</h2><form method='post' action='/boards/rename'><input type='hidden' name='index' value='"+String(chosen)+"'><label for='peerRename'>Jméno tohoto displeje v seznamu</label><input id='peerRename' name='name' maxlength='60' value='"+escapeHtml(name)+"' required><button>Uložit jméno</button></form><p class='hubnote'>Změna názvu v seznamu se ukládá na tomto ESP32. Chceš-li změnit vlastní název vzdáleného ESP32, otevři jeho Nastavení → Název zařízení.</p></section>";
        h+="<div class='hubactions'><a class='button subtle' href='/'>Zpět na aktuální ESP32</a></div>";
      }''')
once('''      h+="<div class='hubhead'><div><h2>"+escapeHtml(boardLabel())+"</h2><small>TramliBern HomeCross · "+escapeHtml(currentAddress())+"</small></div><span class='hubtag'>"+String(WiFi.status()==WL_CONNECTED?"Wi-Fi připojena":"Nastavovací režim")+"</span></div>";''','''      h+="<div class='hubhead'><div><h2>"+escapeHtml(boardLabel())+"</h2><small>Tento displej · TramliBern HomeCross · "+escapeHtml(currentAddress())+"</small></div><span class='hubtag'>"+String(WiFi.status()==WL_CONNECTED?"Wi-Fi připojena":"Nastavovací režim")+"</span></div>";
      h+="<form method='post' action='/device' class='card'><label for='homeBoardName'>Přejmenovat tento displej</label><input id='homeBoardName' name='name' maxlength='60' value='"+escapeHtml(boardLabel())+"' required><button type='submit'>Uložit nové jméno</button></form>";''')
once('''    h+="<p class='hubnote'>Výběrem dalšího displeje se otevře jeho lokální webové ovládání. Všechny změny platí pouze pro vybraný displej.</p></aside>";''','''    h+="<p class='hubnote'>Vlevo je jeden společný seznam. Vpravo jsou odkazy na vybrané zařízení. Jména ostatních zařízení jsou přezdívky uložené v tomto ESP32.</p></aside>";''')
# Old server code reuses name fields. Keep compatibility and no firmware display changes.
assert "<iframe" not in s
assert s.count('server.on("/boards/rename"') == 1
assert 'server.on("/device"' in s
assert 'otaFromHomeWifi()' in s and 'showWifiQR()' in s and 'req.upload.onprogress' in s
assert 'setScreenSleep(' in s and 'void drawBoard()' in s
p.write_text(s,encoding="utf-8")
print("RC33 web-only rename / remote panel patch applied")
