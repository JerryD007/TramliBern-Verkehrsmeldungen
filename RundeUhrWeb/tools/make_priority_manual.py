from pathlib import Path
from reportlab.platypus import SimpleDocTemplate,Paragraph,Spacer,PageBreak,Table,TableStyle
from reportlab.lib.styles import ParagraphStyle
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.lib.colors import HexColor
r=Path(__file__).resolve().parent.parent
pdfmetrics.registerFont(TTFont('DV','/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'))
pdfmetrics.registerFont(TTFont('DVB','/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf'))
style=ParagraphStyle('text',fontName='DV',fontSize=10,leading=15,spaceAfter=9)
head=ParagraphStyle('head',fontName='DVB',fontSize=18,leading=23,spaceAfter=14,textColor=HexColor('#20344f'))
story=[]
def p(s):story.append(Paragraph(s,style))
def h(s):story.append(Paragraph(s,head))
h('RundeUhr 3.2.0-rc11')
p('Český návod • ESP32-C3 Super Mini • GC9A01 240 × 240 • 15. 11. 2026')
h('Zapnout a vypnout prioritu letadel')
p('V sekci Obrazovky je přímo na kartě Nächstes Flugzeug nový přepínač Priorita letadel. Stejný přepínač najdeš v Nové funkce → Nejbližší letadlo. Oba ukazují stejný uložený stav a ukládají změnu ihned; není potřeba další potvrzení tlačítkem.')
p('Zapnuto: nejbližší vhodné letadlo může dostat přednost před ostatními obrazovkami. Vypnuto: priorita je zrušená. Volba V cyklu zůstává samostatná; vypnutí priority neodstraní letadla z běžného střídání.')
p('Priorita se uplatňuje v automatickém režimu. Pokud máš ruční výběr, přepínač vypíše, že čeká na zapnutí automatického přepínání. To zapneš v Ovládání a uložíš běžným tlačítkem.')
p('Režim priority se vybírá zvlášť: Pouze přibližující se nebo Po celou dobu v dosahu. Změnu režimu potvrď tlačítkem Uložit nastavení letadel. Vypnutí a opětovné zapnutí přepínače zvolený režim nemění.')
p('Při odmítnutí uložení se přepínač vrátí na poslední známý stav a zobrazí chybu. Pokud se přeruší spojení, zkontroluj stav po opětovném připojení. Existující nastavení priority, sítě, odpočty, trasy a vzhled displeje se zachovávají.')
h('Nahrání přes Safari')
p('1. Stáhni samostatný RundeUhr_3_2_0_rc11.ino.bin; ZIP není nutné rozbalovat na iPhonu.<br/>2. Připoj iPhone a hodiny do stejné místní sítě.<br/>3. Otevři IP adresu hodin v Safari → Systém → Aktualizace firmware.<br/>4. Vyber BIN a spusť nahrání. Ponech Safari v popředí a hodiny napájené.<br/>5. Po restartu ověř verzi 3.2.0-rc11 a výsledek kontroly otisku firmware.')
h('Aktualizace více hodin')
p('Pokud už všechny vybrané hodiny mají RC6 nebo novější, můžeš použít Systém → Aktualizovat více hodin. Otevři místní IPv4 adresu, vyber BIN, stiskni Najít hodiny a označ cíle. Aktualizace probíhají postupně; při chybě se dávka zastaví. Zařízení, ze kterého je aplikace otevřená, se aktualizuje poslední. Starší verze nejprve aktualizuj jednotlivě přímo na RC11.')
p('Uložené Wi-Fi sítě, nastavení, víkendová animace, větší půllitry a hromadná OTA aktualizace jsou zachovány. Rozdělení Flash se nemění. Pracovní větev je testovací; firmware nebyl vzdáleně nahrán do tvých hodin.')
story.append(PageBreak())
h('Arduino IDE na Macu: přesné nastavení')
p('Pro běžnou aktualizaci používej OTA. Tento postup slouží pro kompilaci nebo obnovu přes USB. Otevři RundeUhr/RundeUhr.ino z úplného rozbaleného ZIP; všechny soubory ve složce musí zůstat pohromadě.')
p('Arduino IDE → Settings → Additional Boards Manager URLs:<br/><font size="8">https://espressif.github.io/arduino-esp32/package_esp32_index.json</font>')
p('Boards Manager: esp32 by Espressif Systems, verze 3.3.0.<br/>Library Manager: Adafruit GFX Library 1.12.1; Adafruit GC9A01A 1.1.0; Adafruit BusIO 1.17.2; ArduinoJson 7.4.2.')
rows=[['Tools','Hodnota'],['Board','ESP32C3 Dev Module'],['USB CDC On Boot','Enabled'],['CPU Frequency','160 MHz'],['Flash Size','4 MB (32 Mb)'],['Flash Mode / Frequency','DIO / 40 MHz'],['Partition Scheme','Minimal SPIFFS (1.9 MB APP with OTA)'],['Upload Speed','115200'],['Erase All Flash Before Sketch Upload','Disabled'],['Port','USB port připojeného ESP32-C3']]
t=Table([[Paragraph(a,style),Paragraph(b,style)] for a,b in rows],colWidths=[240,255]);t.setStyle(TableStyle([('VALIGN',(0,0),(-1,-1),'TOP'),('BACKGROUND',(0,0),(-1,0),HexColor('#e8edf4')),('LINEBELOW',(0,0),(-1,-1),.3,HexColor('#cad2dd'))]));story.append(t)
p('Soubor partitions.csv ve sketchi je závazný: dva OTA oddíly po 0x1E0000 = 1 966 080 B, NVS na 0x9000 o velikosti 0x5000. Neměň rozdělení ani nezapínej mazání Flash.')
p('Zapojení GC9A01: VCC → 3,3 V; GND → G; SCL → GPIO4; SDA → GPIO6; DC → GPIO5; CS → GPIO7; RST → GPIO3.')
n=(r/'build/RundeUhr.ino.bin').stat().st_size
p(f'BIN: {n:,} B. OTA rezerva: 1 966 080 − {n:,} = {1966080-n:,} B. Podrobnosti kompilace a statické RAM jsou v přiloženém protokolu. Výsledek kompilace není měřením skutečné volné RAM během TLS.')
story.append(PageBreak())
h('Ověření a hranice testů')
p('Nové testy prohlížeče ověřují přepínače v obou sekcích, okamžité uložení, zachování režimu a cyklu, načtení po obnovení stránky a návrat přepínače při chybě zápisu. Testuje se WebKit a Chromium s napodobeným API ESP32.')
p('Lokální kompilace a kontrola BIN ověřují ESP32-C3, hash a kapacitu OTA. Test skutečné serverové logiky ověřuje povolení jen nedávno nalezených hodin ze stejné podsítě a odmítnutí jiných webových originů. Přes propojení není přístup k nastavení sítí ani úplnému výpisu diagnostiky.')
p('Testy prohlížečů s napodobenými ESP ověřují pořadí aktualizací, průběh po blocích, samostatné tokeny a identity, kontrolu velikosti a rodiny firmware, restart a hash, přerušený blok, ztracenou odpověď na dokončení, chybný hash, již aktuální firmware a zastavení dávky. Protokoly jsou v ZIP.')
p('Tyto testy nenahrazují skutečnou Wi-Fi, rychlost zápisu do Flash ani chování Safari na konkrétním iPhonu. Skutečný vícezařízení OTA test zatím neproběhl. Zálohy zdrojů a BIN předchozích verzí jsou součástí balíčku.')
p('Hromadná aktualizace podporuje stejnou rodinu RundeUhr pro ESP32-C3 a stejné rozdělení Flash. Není určena pro TramliBern, Tramvaj Praha ani WetterHimmel. Výběr zařízení neslouží k hromadnému přepisování jejich individuálních nastavení.')
p('Světový čas není součástí RC11; zůstává samostatným požadavkem.')
SimpleDocTemplate(str(r/'deliverables_3_2_rc11/RundeUhr_3_2_0_rc11_Navod_CZ.pdf'),rightMargin=48,leftMargin=48,topMargin=45,bottomMargin=45).build(story)
