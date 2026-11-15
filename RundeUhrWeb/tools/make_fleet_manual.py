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
h('RundeUhr 3.2.0-rc6')
p('Český návod • ESP32-C3 Super Mini • GC9A01 240 × 240 • 15. 11. 2026')
h('Hromadná aktualizace přes Safari')
p('RC6 umožňuje vybrat více RundeUhr, jednou zvolit soubor BIN a aktualizovat vybraná zařízení postupně. Každé zařízení má vlastní stav. Aplikace kontroluje identitu, kompatibilitu, kapacitu OTA a po restartu otisk nahraného firmware.')
h('První instalace')
p('Nahraj RC6 do každých hodin jednotlivě přes dosavadní Systém → Aktualizace firmware. Starší verze nemají rozhraní potřebné pro hromadné ovládání. Ponech nastavení Flash a sítě beze změny.')
h('Další aktualizace jedním spuštěním')
p('1. Připoj všechny hodiny a iPhone nebo Mac do stejné místní sítě. Zařízení se musí vzájemně vidět; izolovaná síť pro hosty nemusí vyhovovat.<br/>2. Otevři místní IP adresu jedněch hodin v Safari. Pokud používáš adresu .local, tlačítko Najít hodiny nabídne odkaz na IP adresu.<br/>3. V sekci Systém vyber nahoře jeden soubor .ino.bin.<br/>4. V panelu Aktualizovat více hodin stiskni Najít hodiny a zaškrtni požadovaná zařízení.<br/>5. Stiskni Aktualizovat vybrané hodiny a zkontroluj seznam názvů a IP adres v potvrzení.<br/>6. Ponech Safari v popředí a hodiny napájené. Sleduj výsledek u každého zařízení.')
p('Před prvním zápisem se zkontrolují všechny vybrané hodiny. Aktualizace probíhají po jedné; zařízení, ze kterého je aplikace otevřená, se aktualizuje poslední. Pokud už má cílový otisk firmware, přeskočí se jako již aktuální.')
p('Zastavit po aktuálních hodinách dokončí právě zpracovávané zařízení a další nespustí. Při chybě nebo neověřeném výsledku se dávka zastaví. Přerušený zápis se automaticky neopakuje. Po dokončení nebo chybě obnov stránku a ověř verze, než spustíš další dávku.')
p('Pokud seznam další hodiny neukáže, počkej 15 sekund a obnov hledání. Zkontroluj stejnou síť a dostupnost jednotlivých IP adres v Safari. Pokud Safari žádá přístup do místní sítě, pro tuto funkci jej povol. Není potřeba účet, cloud ani zapnutý Mac při práci z iPhonu.')
p('Zachovány jsou uložené sítě, nastavení, RC5 oprava kontroly RAM, půllitry i přípitek. RC6 není fyzicky ověřena na více ESP32; první skutečnou dávku je potřeba otestovat na tvých hodinách.')
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
p('Lokální kompilace a kontrola BIN ověřují ESP32-C3, hash a kapacitu OTA. Test skutečné serverové logiky ověřuje povolení jen nedávno nalezených hodin ze stejné podsítě a odmítnutí jiných webových originů. Přes propojení není přístup k nastavení sítí ani úplnému výpisu diagnostiky.')
p('Testy prohlížečů s napodobenými ESP ověřují pořadí aktualizací, průběh po blocích, samostatné tokeny a identity, kontrolu velikosti a rodiny firmware, restart a hash, přerušený blok, ztracenou odpověď na dokončení, chybný hash, již aktuální firmware a zastavení dávky. Protokoly jsou v ZIP.')
p('Tyto testy nenahrazují skutečnou Wi-Fi, rychlost zápisu do Flash ani chování Safari na konkrétním iPhonu. Skutečný vícezařízení OTA test zatím neproběhl. Zálohy zdrojů a BIN předchozích verzí jsou součástí balíčku.')
p('Hromadná aktualizace podporuje stejnou rodinu RundeUhr pro ESP32-C3 a stejné rozdělení Flash. Není určena pro TramliBern, Tramvaj Praha ani WetterHimmel. Výběr zařízení neslouží k hromadnému přepisování jejich individuálních nastavení.')
p('Světový čas není součástí RC6; zůstává samostatným požadavkem.')
SimpleDocTemplate(str(r/'deliverables_3_2_rc6/RundeUhr_3_2_0_rc6_Navod_CZ.pdf'),rightMargin=48,leftMargin=48,topMargin=45,bottomMargin=45).build(story)
