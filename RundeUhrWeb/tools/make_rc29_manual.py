from pathlib import Path
import json
from reportlab.platypus import SimpleDocTemplate,Paragraph,PageBreak,Table,TableStyle
from reportlab.lib.styles import ParagraphStyle
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.lib.colors import HexColor
r=Path(__file__).resolve().parents[2];out=r/'deliverables_rc29';out.mkdir(exist_ok=True)
for name,file in [('DV','DejaVuSans.ttf'),('DVB','DejaVuSans-Bold.ttf')]:pdfmetrics.registerFont(TTFont(name,'/usr/share/fonts/truetype/dejavu/'+file))
s=ParagraphStyle('body',fontName='DV',fontSize=10,leading=14,spaceAfter=10)
h=ParagraphStyle('heading',fontName='DVB',fontSize=17,leading=22,spaceAfter=14,textColor=HexColor('#244369'));story=[]
def p(t):story.append(Paragraph(t,s))
def title(t):story.append(Paragraph(t,h))
title('RundeUhr RC29 - rychlejší posun zpráv')
p('Český návod • 18. 11. 2026 • ESP32-C3 Super Mini • GC9A01 240 × 240')
p('<b>Změna RC29:</b> rychlost zpráv zvýšena z 90 na 135 pixelů/s, tedy o 50 %. Délka průjezdu se přepočítává podle stejné rychlosti. Počáteční a závěrečná pauza zůstávají 1,5 sekundy. Krátké titulky zůstávají 3 sekundy. Ochrana proti předčasnému přepnutí při zdržení zůstává.')
p('<b>Bitcoin:</b> červená křivka v pozadí ukazuje posledních 32 úspěšných měření ve zvolené měně. Při pravidelném načítání po 2 minutách představuje přibližně 62 minut. Časová osa odpovídá skutečným časům měření. Nejde o historická data burzy: po restartu nebo změně měny začíná sběr znovu a graf se objeví po druhém platném měření. Výpadky nevkládají nulové ceny. Neprovádějí se další síťové požadavky kvůli grafu.')
p('<b>Oprava RC29:</b> přenos displeje správně čte dva pixely z každého bajtu 4bitového bufferu. RC27 nepoužívej; její nový přenos předpokládal jiný formát. Test RC29 používá skutečnou třídu PaletteCanvas a kontrolu přístupů do paměti.')
p('<b>Vykreslování zpráv:</b> během pohybu zpráv se překresluje pouze pruh 184 × 32 pixelů. Odstraněn je pravidelný úplný přenos po jedné sekundě. SPI přenos používá jednu transakci a jeden adresní obdélník. Při zdržení hlavní smyčky se doba průjezdu prodlouží; zdržení nespotřebuje celý zbývající titulek. Rychlost zůstává 135 pixelů/s.')
p('<b>Rakety ve střídání:</b> budoucí načtené starty během 7 dní (10 080 minut). Povoleny jsou plánované, nepotvrzené a odložené starty s budoucím termínem. Přesnost na hodinu není překážkou. Neurčitý termín je označen Zeit ungefaehr, nepotvrzený Unbestaetigt. Zrušené, proběhlé a starty bez použitelného termínu se vynechají. Zůstávají nejvýše tři položky dodané API.')
p('<b>Raketová priorita:</b> pouze stav Go, přesnost MIN, start v příštích 60 minutách. Okno běžného střídání tuto prioritu nerozšiřuje.')
p('<b>Zemětřesení ve střídání:</b> posledních 24 hodin, od magnitudy 4,5, celosvětově. <b>Priorita:</b> od magnitudy 6,0 a nejvýše 60 minut od události. Bez odpovídajících aktuálních dat se obrazovka přeskočí.')
p('<b>Zprávy:</b> nejvýše 24 hodin od vydání článku, nikoli od stažení. Zobrazují čas a zdroj. Jeden titulek dokončí celý průjezd, potom se pokračuje ve střídání. Při dalším návratu se použije další způsobilý titulek. Krátký titulek se zobrazí na 3 sekundy; dlouhý podle délky při 135 pixelech/s včetně pauz. Zprávy nemají prioritu.')
p('Priority přeruší běžné střídání na 20 sekund, s odstupem nejméně 2 minut. Paměť již oznámených událostí přežije restart. Ukládá nejvýše 64 dosud relevantních událostí; při naplnění raději potlačí další prioritu, než aby opakovala již oznámenou událost. Ruční výběr má přednost. Není to systém nouzového varování.')
p('<b>Přechod z RC25 a starší:</b> uvedené filtry se převedou automaticky (rakety 10 080, zemětřesení 1 440 / M4,5 / 0 km, zprávy 1 440). Ostatní nastavení, uložené Wi-Fi, volba zdroje i zapnuté obrazovky zůstanou. Při aktualizaci z RC26 se hodnoty filtrů nemění. U požadovaných obrazovek musí být zapnuté Im Wechsel a celkové automatické střídání.')
p('<b>Diagnostika RC29:</b> System → Detaillierte Diagnose nově uvádí Nachrichten: längste Bildpause (největší prodleva mezi snímky zpráv) a Nachrichten: Bildpausen über 50 ms (počet prodlev nad 50 ms). Hodnoty jsou od restartu. Pomohou rozlišit přetrvávající blokování; nezaručují plynulost fyzického LCD.')
p('<b>ISS:</b> opraveno pořadí inicializace HTTP a ověřeného TLS připojení, aby HTTP inicializace nezavírala právě ověřené spojení. Kontrola certifikátu zůstává povinná. Tato oprava nepotvrzuje odstranění všech chyb certifikačního řetězce; případné TLS -9984 je nutné ověřit na zařízení.')
story.append(PageBreak());title('Nahrání přes Safari a návrat')
for t in ['1. Stáhni samostatný RundeUhr_3_2_0_rc29.ino.bin. Na iPhonu se nic nerozbaluje.', '2. Připoj telefon ke stejné Wi-Fi jako hodiny a otevři místní IP v Safari. V System si nejprve ulož zálohu nastavení.', '3. Firmware-Update: vyber BIN a spusť nahrávání. Neodpojuj napájení a nech Safari otevřené až do dokončení.', '4. Po restartu obnov Safari a ověř firmware 3.2.0-rc29. Na stránce událostí ověř nové hodnoty filtrů a přepínače Im Wechsel.', '5. Pro návrat nahraj záložní RC28 z ZIP. RC28 podporuje stejný formát nastavení a Wi-Fi zůstávají. Původní uživatelskou konfiguraci lze obnovit z vlastní zálohy.']:p(t)
p('Kompletní ZIP obsahuje zdrojový projekt, výsledky testů, PDF, nový BIN a zálohu RC28. Hlavní větev ani firmware tvých hodin nebyly automaticky změněny.')
title('Arduino IDE - přesné nastavení')
p('Boards Manager URL:<br/><font size="8">https://espressif.github.io/arduino-esp32/package_esp32_index.json</font>')
p('Boards Manager: <b>esp32 by Espressif Systems 3.3.0</b>.<br/>Library Manager: <b>Adafruit GFX Library 1.12.1</b>, <b>Adafruit GC9A01A 1.1.0</b>, <b>Adafruit BusIO 1.17.2</b>, <b>ArduinoJson 7.4.2</b>.')
rows=[['Tools','Hodnota'],['Board','ESP32C3 Dev Module'],['USB CDC On Boot','Enabled'],['CPU Frequency','160 MHz'],['Flash Size','4 MB (32 Mb)'],['Flash Mode / Frequency','DIO / 40 MHz'],['Partition Scheme','Minimal SPIFFS (1.9 MB APP with OTA)'],['Upload Speed','115200'],['Erase All Flash Before Sketch Upload','Disabled'],['Port','USB port připojeného ESP32-C3']]
t=Table([[Paragraph(a,s),Paragraph(b,s)] for a,b in rows],colWidths=[225,270]);t.setStyle(TableStyle([('VALIGN',(0,0),(-1,-1),'TOP'),('BACKGROUND',(0,0),(-1,0),HexColor('#e8edf4')),('LINEBELOW',(0,0),(-1,-1),.3,HexColor('#cad2dd'))]));story.append(t)
story.append(PageBreak());title('Kompilace, paměť a test na hodinách')
p('Otevři zdroje/RundeUhrWeb/RundeUhr/RundeUhr.ino. Přiložený partitions.csv zachovává dvě OTA oblasti po 1 966 080 B a NVS na 0x9000 o délce 0x5000. Nepoužívej mazání celé Flash. Pro USB použij Upload; pro běžný update postačí Safari.')
p('Zapojení: VCC → 3,3 V; GND → G; SCL → GPIO4; SDA → GPIO6; DC → GPIO5; CS → GPIO7; RST → GPIO3.')
b=out/'RundeUhr_3_2_0_rc29.ino.bin'
if b.exists():p(f'BIN: {b.stat().st_size:,} B. Rezerva OTA: 1 966 080 − {b.stat().st_size:,} = {1966080-b.stat().st_size:,} B.'.replace(',',' '))
report=out/'Vysledky_testu_RC29.json'
if report.exists():
 d=json.loads(report.read_text());p(f'Statická RAM: {d["static_ram"]} B. Nejde o měření volné RAM za provozu TLS. Podrobnosti, SHA-256 a logy jsou v ZIP.')
p('Počítačové testy kontrolují hranice stáří a magnitudy, přesnost a stav startů, oddělenou prioritu, migraci konfigurace a zachování přístupových údajů, střídání titulků a jejich vykreslení. Testy webu používají simulované API ESP32. Výsledky skutečné kompilace ESP32-C3 a jednotlivých kontrol jsou přiložené v ZIP.')
p('<b>Firmware nebyl testován na fyzickém ESP32.</b> Nový test porovnává částečné a úplné vykreslení pixel po pixelu, SPI obdélník a simulované několikasekundové zdržení. Automatické testy a kompilace nenahrazují dlouhodobý provoz na hodinách, skutečnou OTA aktualizaci, přepínání Wi-Fi a TLS handshake na ESP32.')
for t in ['Na jedněch hodinách ověř zachování názvu, Wi-Fi, ostatních nastavení a rozsvícení displeje po restartu.', 'V Im Wechsel nech zapnuté rakety i zprávy. U raket se musí objevit i budoucí start s přesností Stunde; při chybě zdroje se obrazovka přeskočí.', 'U zpráv sleduj celý průjezd dlouhého titulku. Po dalším návratu se musí ukázat následující zpráva. Aktualizace RSS během průjezdu nemá měnit právě čtený titulek.', 'Zkontroluj ISS TLS-Detail po několika načteních. Případnou novou chybu vyfoť včetně času posledního pokusu a údajů RAM.']:p(t)
def footer(c,d):c.setFont('DV',8);c.setFillColor(HexColor('#627389'));c.drawString(42,25,'RundeUhr 3.2.0-rc29 • Český návod');c.drawRightString(553,25,str(d.page))
SimpleDocTemplate(str(out/'RundeUhr_3_2_0_rc29_Navod_CZ.pdf'),pagesize=(595,842),leftMargin=42,rightMargin=42,topMargin=38,bottomMargin=42).build(story,onFirstPage=footer,onLaterPages=footer)
