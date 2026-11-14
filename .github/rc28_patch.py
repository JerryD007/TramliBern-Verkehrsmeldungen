from pathlib import Path
import sys
p=Path(sys.argv[1]); s=p.read_text()
def one(a,b):
 global s
 assert s.count(a)==1, "patch target mismatch: "+a[:70]
 s=s.replace(a,b)
one('constexpr const char *VERSION = "TramliBern 4.0 RC27 HomeCross";','constexpr const char *VERSION = "TramliBern 4.0 RC28 HomeCross";')
one('void setScreenSleep(bool sleeping) {','void applyBrightness(); // Forward declaration; required before setScreenSleep()\nvoid setScreenSleep(bool sleeping) {')
one('if (title != "Menü") html += "<nav aria-label=\'Navigation\'><a href=\'/\'>← Menü</a><a href=\'/boards\'>▣ Meine Boards</a></nav>";',"""if (title != "Menü") html += "<nav aria-label='Navigation'><a href='/'>← Menü</a><a href='/boards'>▣ Meine Boards</a></nav>";
  // Always reachable on every Safari page, including when the physical display is dark.
  html += "<form action='/screen-on' method='post'><button class='full' style='background:#137c54'>PROBUDIT DISPLEJ</button></form>";""")
one('Version 4.0 RC26 HomeCross · 3 Seiten','Version 4.0 RC28 HomeCross · 3 Seiten')
one("""h += "<form method='post' action='/sleep-config'><label>Automatisch nach Minuten (0 = AUS, max. 240)</label><input type='number' name='min' min='0' max='240' value='"+String(sleepMinutes)+"' required><button>Speichern</button></form>";""","""h += "<form method='post' action='/sleep-config'><label><input type='checkbox' name='enabled' value='yes'"+String(sleepMinutes>0?" checked":"")+"> Automatické zhasnutí zapnuto</label><label>Doba nečinnosti (minuty)</label><input type='number' name='min' min='1' max='240' value='"+String(sleepMinutes?sleepMinutes:10)+"' required><button>Uložit nastavení</button></form>";""")
one("""server.on("/sleep-config",HTTP_POST,[](){ int m; if(!numericArg("min",0,240,m)){showMessage("Ungueltiger Minutenwert",400);return;} sleepMinutes=m; prefs.putUShort("sleepMin",sleepMinutes);lastUserActivity=millis();server.sendHeader("Location","/display");server.send(303); });""","""server.on("/sleep-config",HTTP_POST,[](){ int m; if(!numericArg("min",1,240,m)){showMessage("Ungueltiger Minutenwert",400);return;} sleepMinutes=server.arg("enabled")=="yes" ? m : 0; prefs.putUShort("sleepMin",sleepMinutes);lastUserActivity=millis();server.sendHeader("Location","/display");server.send(303); });""")
one("""    drawLine(clockMinutes(nightStart)+" - "+clockMinutes(nightEnd),12,217,WHITE,2);
    drawLine("Speichert automatisch. Ohne Uhr: Tageswert.",12,253,WHITE,1);""","""    drawLine(clockMinutes(nightStart)+" - "+clockMinutes(nightEnd),12,217,WHITE,2);
    touchButton("ZHASNOUT",246,211,222,39);
    drawLine("Dotyk znovu probudi. Auto: "+String(sleepMinutes?String(sleepMinutes)+" min":"AUS"),12,253,WHITE,1);""")
one("""  if(touchMode==13){
    if(x>=12&&x<468&&y>=276""","""  if(touchMode==13){
    if(x>=246&&x<468&&y>=211&&y<250){setScreenSleep(true);return true;}
    if(x>=12&&x<468&&y>=276""")
p.write_text(s); print("RC28 patched")
