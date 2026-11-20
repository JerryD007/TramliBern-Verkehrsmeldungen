from pathlib import Path
import json
from reportlab.platypus import SimpleDocTemplate, Paragraph, PageBreak, Table, TableStyle, Spacer
from reportlab.lib.styles import ParagraphStyle
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.lib.colors import HexColor
root=Path(__file__).resolve().parents[3];out=root/'deliverables_rc31';out.mkdir(exist_ok=True)
for name,file in [('DV','DejaVuSans.ttf'),('DVB','DejaVuSans-Bold.ttf')]:pdfmetrics.registerFont(TTFont(name,'/usr/share/fonts/truetype/dejavu/'+file))
s=ParagraphStyle('body',fontName='DV',fontSize=10,leading=14,spaceAfter=10)
h=ParagraphStyle('title',fontName='DVB',fontSize=20,leading=25,spaceAfter=18,textColor=HexColor('#087b86'))
sub=ParagraphStyle('sub',parent=h,fontSize=13,leading=17,spaceBefore=12,spaceAfter=9)
story=[]
def p(t):story.append(Paragraph(t,s))
def title(t):story.append(Paragraph(t,h))
def heading(t):story.append(Paragraph(t,sub))
title('RundeUhr RC31\n<br/>Wi-Fi přes QR kód')
p('Český návod | 20. 11. 2026 | ESP32-C3 Super Mini + GC9A01 240 × 240')
p('QR kód na displeji obsahuje název a heslo nastavovací Wi-Fi hodin. Připojí telefon k hodinám; heslo nové domácí sítě zadáš až na jejich místní stránce. Není potřeba účet ani další aplikace.')
heading('První zapnutí nebo nová domácnost')
for text in ['1. Zapni hodiny. Bez uložené sítě se zobrazí WLAN Setup s QR kódem. Pokud jsou uložené sítě nedostupné, hodiny nejprve zkoušejí připojení; po přibližně 45 sekundách se zobrazí QR kód.', '2. Otevři na iPhonu Fotoaparát, namiř na QR kód a klepni na nabídku připojení k Wi-Fi. Potvrď připojení. Fotku není nutné pořizovat.', '3. Pokud se nastavení neotevře samo, otevři Safari a napiš <b>http://192.168.4.1</b>. Telefon musí zůstat připojený k Wi-Fi hodin, i když tato síť nemá internet.', '4. Na stránce WLAN vyber domácí síť, zadej její heslo a stiskni <b>Speichern und verbinden</b>. Volba Passwort anzeigen zobrazí nebo skryje heslo.', '5. Po úspěšném připojení QR kód zmizí a vrátí se běžná obrazovka. Uložené sítě zůstávají zachované; hodiny umějí uchovat nejvýše osm sítí.']:p(text)
heading('Když už hodiny fungují')
p('Nastavovací obrazovku lze vyvolat z místní webové aplikace tlačítkem pro nastavení WLAN. Při ručním vyvolání je zobrazena 60 sekund. Při trvalém výpadku sítě zůstává QR viditelný až do připojení. Krátký výpadek do 45 sekund nepřepíná displej na QR.')
p('Automatické otevření stránky závisí na iOS; záložní adresa je proto stále pod QR kódem. Nastavovací síť zůstává podle dosavadního chování dostupná i po připojení k domácí Wi-Fi.')
story.append(PageBreak());title('Aktualizace a Arduino IDE')
heading('Nahrání z iPhonu přes Safari')
for text in ['1. Stáhni samostatný <b>RundeUhr_3_2_0_rc31.ino.bin</b> do aplikace Soubory. ZIP není pro aktualizaci potřeba rozbalovat.', '2. Připoj iPhone ke stejné domácí Wi-Fi jako hodiny. Otevři jejich místní IP adresu v Safari. V System si nejprve stáhni zálohu nastavení.', '3. V Firmware-Update vyber BIN a spusť nahrávání. Safari nech otevřené a nevypínej napájení.', '4. Po restartu ověř verzi <b>3.2.0-rc31</b>, uložené sítě a nastavení. Na zařízení nic automaticky nenahrávám.', '5. Pro návrat použij BIN RC29 ze záložního balíčku v ZIP. Nemaž celou Flash. Záloha uživatelských nastavení vytvořená před aktualizací umožní jejich obnovu.']:p(text)
heading('Přesné nastavení pro vlastní kompilaci')
p('Boards Manager URL:<br/><font size="8">https://espressif.github.io/arduino-esp32/package_esp32_index.json</font><br/>Boards Manager: <b>esp32 by Espressif Systems 3.3.0</b>.')
p('Library Manager: <b>Adafruit GFX Library 1.12.1</b>, <b>Adafruit GC9A01A 1.1.0</b>, <b>Adafruit BusIO 1.17.2</b>, <b>ArduinoJson 7.4.2</b>. QR knihovna je již ve zdrojích, nic dalšího pro QR neinstaluj.')
rows=[['Tools','Hodnota'],['Board','ESP32C3 Dev Module'],['USB CDC On Boot','Enabled'],['CPU Frequency','160 MHz'],['Flash Size','4 MB (32 Mb)'],['Flash Mode / Frequency','DIO / 40 MHz'],['Partition Scheme','Minimal SPIFFS (1.9 MB APP with OTA)'],['Upload Speed','115200'],['Erase All Flash Before Sketch Upload','Disabled'],['Port','USB port připojeného ESP32-C3']]
t=Table([[Paragraph(a,s),Paragraph(b,s)] for a,b in rows],colWidths=[225,285]);t.setStyle(TableStyle([('VALIGN',(0,0),(-1,-1),'TOP'),('BACKGROUND',(0,0),(-1,0),HexColor('#e5f4f5')),('LINEBELOW',(0,0),(-1,-1),.3,HexColor('#cbd4d7')),('BOTTOMPADDING',(0,0),(-1,-1),3)]));story.append(t)
story.append(PageBreak());title('Zdrojový projekt a ověření')
p('Otevři <b>zdroje/RundeUhrWeb/RundeUhr/RundeUhr.ino</b>. Všechny soubory ve složce RundeUhr ponech pohromadě, včetně qrcodegen.c, qrcodegen.h a partitions.csv. Pro USB použij Upload; pro běžné aktualizace stačí Safari.')
p('Zapojení: VCC → 3,3 V; GND → G; SCL → GPIO4; SDA → GPIO6; DC → GPIO5; CS → GPIO7; RST → GPIO3.')
p('Rozdělení Flash se nemění: NVS na 0x9000, délka 0x5000; dva OTA oddíly po 1 966 080 B. Aktualizace neprovádí vymazání Wi-Fi ani uživatelských nastavení.')
b=out/'RundeUhr_3_2_0_rc31.ino.bin'
if b.exists():p(f'Velikost BIN: {b.stat().st_size:,} B.<br/>Rezerva OTA: 1 966 080 - {b.stat().st_size:,} = {1966080-b.stat().st_size:,} B.'.replace(',',' '))
report=out/'Vysledky_testu_RC31.json'
if report.exists():
 d=json.loads(report.read_text());p(f'Statická RAM při kompilaci: {d["static_ram"]:,} B. Nejde o volnou paměť za provozu TLS.'.replace(',',' '))
heading('Co bylo kontrolováno na počítači')
p('Kompilace pro ESP32-C3, typ výsledného BIN, SHA-256, OTA kapacita a zachování oddílů. Regresní testy logiky a vykreslování. QR testy kontrolují speciální znaky, dlouhý název sítě, hranice bufferů a kruhového displeje. Nezávislý dekodér ZXing čte QR přímo z vykresleného obrázku 240 × 240. Webové testy používají WebKit a Chromium se simulovaným API, včetně otevření WLAN z adresy 192.168.4.1 a odeslání formuláře.')
p('Součástí je také oprava rezervace RSS parseru před TLS z RC30. Existující rychlost zpráv 135 pixelů/s, graf Bitcoinu, filtry událostí a funkce OTA zůstávají zachované. Podrobné výsledky a logy jsou v ZIP.')
heading('Co zbývá ověřit na skutečných hodinách')
p('<b>Tato verze nebyla testována na fyzickém ESP32 ani skenována iPhonem z LCD.</b> Počítačové testy nepotvrzují dosah Wi-Fi, čitelnost při odlescích ani automatické otevření portálu v konkrétní verzi iOS.')
for text in ['Po OTA ověř zachování nastavení a rozsvícení displeje po restartu.', 'Vyzkoušej QR ručně. Připoj iPhone, otevři nastavení a přidej druhou síť; potom ověř automatický návrat k dostupné uložené síti.', 'Ověř krátký výpadek i nedostupnost sítě přes 45 sekund. Při trvalém výpadku musí QR zůstat viditelný.', 'Ověř návrat běžného displeje po připojení, zprávy, rakety a běžný dlouhodobý provoz.']:p(text)
def footer(c,d):
 c.setFont('DV',8);c.setFillColor(HexColor('#50636c'));c.drawString(42,24,'RundeUhr 3.2.0-rc31 | Český návod');c.drawRightString(553,24,str(d.page))
SimpleDocTemplate(str(out/'RundeUhr_3_2_0_rc31_Navod_CZ.pdf'),pagesize=(595,842),leftMargin=42,rightMargin=42,topMargin=36,bottomMargin=42).build(story,onFirstPage=footer,onLaterPages=footer)
