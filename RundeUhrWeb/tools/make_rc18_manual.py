from pathlib import Path
from reportlab.platypus import SimpleDocTemplate,Paragraph,PageBreak,Table,TableStyle
from reportlab.lib.styles import ParagraphStyle
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.lib.colors import HexColor
r=Path(__file__).resolve().parents[2];out=r/'deliverables_rc18';out.mkdir(exist_ok=True)
for name,file in [('DV','DejaVuSans.ttf'),('DVB','DejaVuSans-Bold.ttf')]:pdfmetrics.registerFont(TTFont(name,'/usr/share/fonts/truetype/dejavu/'+file))
s=ParagraphStyle('body',fontName='DV',fontSize=10,leading=15,spaceAfter=10);h=ParagraphStyle('h',fontName='DVB',fontSize=18,leading=23,spaceAfter=14,textColor=HexColor('#244369'));story=[]
def p(t):story.append(Paragraph(t,s))
def title(t):story.append(Paragraph(t,h))
title('RundeUhr RC18 - Zemětřesení')
p('Český návod • ESP32-C3 Super Mini • GC9A01 240 × 240 • 16. 11. 2026')
p('Nová obrazovka <b>Erdbeben</b> a německé ovládání webové aplikace. Vlastní názvy událostí, míst a zařízení se nepřekládají. Uložené Wi-Fi a nastavení zůstávají zachované. Zapojení a rozdělení Flash se nemění.')
title('Aktualizace z iPhonu přes Safari')
for t in ['1. Stáhni samostatný <b>RundeUhr_3_2_0_rc18.ino.bin</b>. Pro OTA nemusíš rozbalovat ZIP.','2. Připoj iPhone ke stejné Wi-Fi jako hodiny. Otevři jejich místní adresu, například http://10.128.0.47.','3. V RC17 nejprve stáhni vlastní zálohu v Systém → Záloha nastavení. Wi-Fi hesla nejsou součástí této zálohy.','4. V Systém → Aktualizace firmware vyber BIN a potvrď nahrání. Safari nech v popředí a nepřerušuj napájení hodin.','5. Vyčkej na restart a ověření firmware. Obnov stránku. Musí být uvedeno <b>3.2.0-rc18</b> a německé menu.','6. <b>Ansichten → Erdbeben → Auswählen</b> zobrazí zemětřesení. <b>Im Wechsel</b> zapne zařazení do cyklu. Výchozí cyklus se automaticky nemění.']:p(t)
p('Další cesta: <b>Erweiterungen → Erdbeben anzeigen</b>. Diagnostika: <b>System → Detaillierte Diagnose</b>. Miniatura na kartě používá příklad; skutečný snímek displeje najdeš v <b>Übersicht</b>.')
p('<b>Více hodin:</b> System → Mehrere Uhren aktualisieren. Zařízení musí být ve stejné místní síti a mít RC6 nebo novější. Aktualizace probíhají postupně; při chybě se zastaví.')
p('<b>Záloha v ZIP:</b> kompletní předchozí RC17, nikoli tvoje soukromá nastavení. Pro návrat vypni Erdbeben v cyklu, vyber jinou obrazovku, nech změnu alespoň 3 sekundy uložit a teprve potom nahraj RC17. Starší firmware nezná novou obrazovku ID 14.')
story.append(PageBreak());title('Zobrazení a kontrola provozu')
p('USGS: celosvětová zemětřesení M4,5+ za posledních 24 hodin. Nejvýše pět nejnovějších událostí se střídá po osmi sekundách, dokud je obrazovka aktivní. Ukazuje magnitudu, místo, čas Europe/Zurich a hloubku v km. Nejde o předpověď ani nouzové varování.')
p('Názvy míst se přebírají z USGS, zpravidla anglicky. Nejsou strojově překládány. Na displeji se převádějí na čitelnou latinku a zobrazují nejvýše na třech řádcích. Události pod M4,5 nejsou zahrnuty.')
p('Načítání každých 5 minut pouze při ručním výběru nebo zapnutí do cyklu. Také po chybě je další pokus za 5 minut. Platný prázdný seznam: <b>Keine Ereignisse</b>. Data zdroje starší než 15 minut: <b>Nicht aktuell</b>. Výpadek sítě: <b>Offline</b>. Poslední platné údaje se při chybě zachovají.')
p('Ochrana RAM omezuje odpověď na 24 KiB. Příliš velká, neúplná nebo neplatná odpověď nenahradí poslední data. Při mimořádně velkém počtu událostí může být hlášena chyba velikosti odpovědi. HTTPS ověřuje certifikát; načítání běží v síťovém úkolu na pozadí.')
title('Test na skutečných hodinách')
for t in ['Ověř zachování Wi-Fi, vlastních názvů a cyklu. Vyzkoušej původní počasí, letadla, ISS a odpočty.','Zapni Erdbeben a v diagnostice ověř HTTP 200 a čas načtení. Při potížích pošli snímek diagnostiky.','Během načítání přepínej obrazovky a ověř odezvu webu. Zkontroluj volnou RAM a největší blok.','Krátce vypni a zapni napájení: displej se má rozsvítit a Wi-Fi znovu připojit.']:p(t)
p('Testy na počítači kontrolují prázdná, chybná, budoucí a zastaralá data, pořadí pěti událostí a rozměry vykreslení. WebKit a Chromium používají simulované API ESP32. Živé API/TLS se testuje na počítači/CI. <b>RC18 není autorem aktualizace otestovaná na fyzickém zařízení.</b> Výsledky jsou v přiloženém ZIP.')
p('Dokumentace zdroje:<br/><font size="8">https://earthquake.usgs.gov/earthquakes/feed/v1.0/geojson.php<br/>https://earthquake.usgs.gov/earthquakes/feed/v1.0/summary/4.5_day.geojson</font>')
story.append(PageBreak());title('Přesné nastavení Arduino IDE')
p('Boards Manager URL:<br/><font size="8">https://espressif.github.io/arduino-esp32/package_esp32_index.json</font>')
p('Boards Manager: <b>esp32 by Espressif Systems 3.3.0</b>.<br/>Library Manager: <b>Adafruit GFX Library 1.12.1</b>; <b>Adafruit GC9A01A 1.1.0</b>; <b>Adafruit BusIO 1.17.2</b>; <b>ArduinoJson 7.4.2</b>.')
rows=[['Tools','Hodnota'],['Board','ESP32C3 Dev Module'],['USB CDC On Boot','Enabled'],['CPU Frequency','160 MHz'],['Flash Size','4 MB (32 Mb)'],['Flash Mode / Frequency','DIO / 40 MHz'],['Partition Scheme','Minimal SPIFFS (1.9 MB APP with OTA)'],['Upload Speed','115200'],['Erase All Flash Before Sketch Upload','Disabled'],['Port','USB port tvého ESP32-C3']]
t=Table([[Paragraph(a,s),Paragraph(b,s)] for a,b in rows],colWidths=[225,270]);t.setStyle(TableStyle([('VALIGN',(0,0),(-1,-1),'TOP'),('BACKGROUND',(0,0),(-1,0),HexColor('#e8edf4')),('LINEBELOW',(0,0),(-1,-1),.3,HexColor('#cad2dd'))]));story.append(t)
p('Otevři celý sketch zdroje/RundeUhrWeb/RundeUhr/RundeUhr.ino. Přiložený partitions.csv je rozhodující: dvě OTA aplikace po 1 966 080 B, NVS 0x9000 / 0x5000. Nemazat celou Flash.')
p('Zapojení: VCC → 3,3 V; GND → G; SCL → GPIO4; SDA → GPIO6; DC → GPIO5; CS → GPIO7; RST → GPIO3.')
b=out/'RundeUhr_3_2_0_rc18.ino.bin'
if b.exists():p(f'BIN má {b.stat().st_size} B. Rezerva OTA = 1 966 080 − {b.stat().st_size} = {1966080-b.stat().st_size} B.')
p('Statická RAM z kompilace není údaj o volné RAM během HTTPS. Úplné výsledky a SHA-256 jsou součástí ZIP. Soubory pro OTA jsou zkompilované pro ESP32-C3 s uvedenými knihovnami.')
def footer(c,d):c.setFont('DV',8);c.setFillColor(HexColor('#627389'));c.drawString(42,25,'RundeUhr 3.2.0-rc18 • Český návod');c.drawRightString(553,25,str(d.page))
SimpleDocTemplate(str(out/'RundeUhr_3_2_0_rc18_Navod_CZ.pdf'),pagesize=(595,842),leftMargin=42,rightMargin=42,topMargin=38,bottomMargin=42).build(story,onFirstPage=footer,onLaterPages=footer)
