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
h('RundeUhr 3.2.0-rc5')
p('Český návod • ESP32-C3 Super Mini • GC9A01 240 × 240 • 15. 11. 2026')
h('Oprava paměťové kontroly HTTPS')
p('RC5 opravuje příliš velkou zkušební alokaci pro Bitcoin, Currency, letadla a jejich cíl. RC4 zkoušela dva bloky po 18 432 B. Použitá knihovna TLS potřebuje pro každý datový buffer 16 717 B včetně režie. Velikosti se nyní přebírají přímo z přibalené knihovny; kompilace ověřuje konfiguraci.')
p('Rozdíl: 2 × (18 432 − 16 717) = 3 430 B zbytečně požadovaných při původní kontrole. Nejde o trvale ušetřenou RAM. Při roztříštěné paměti může tento rozdíl rozhodnout, zda se povolí požadavek.')
p('Ochranná hranice 65 000 B celkové volné RAM zůstává. Pokud skutečně chybí paměť, načítání se odloží o 15 sekund a poslední hodnota zůstane označená jako neaktuální. Ověřování certifikátů, velikosti TLS bufferů ani síťové zásobníky se nemění.')
p('Výpis ze zařízení potvrdil odložení před HTTP, nikoli chybu API. Neobsahuje mapu všech volných bloků. Proto nelze před nahráním slíbit, že RC5 vyřeší veškeré příčiny nedostatku RAM. Simulace ověřila konkrétní případ zbytečného odmítnutí i zachování ochran.')
h('Nahrání a kontrola na hodinách')
p('1. Stáhni samostatný RundeUhr_3_2_0_rc5.ino.bin do Souborů.<br/>2. Na stejné Wi-Fi otevři místní adresu hodin v Safari.<br/>3. Systém → Aktualizace firmware → vyber BIN → nahraj a počkej na restart.<br/>4. Ověř verzi 3.2.0-rc5. Nech Bitcoin, Currency a letadla zapnuté v cyklu; vyčkej 1–2 minuty.<br/>5. Ověř, zda se načtou tři měnové páry a obnoví Bitcoin. Bez letadel v dosahu může být správným výsledkem prázdný seznam, nikoli chyba.')
p('Pokud problém zůstane, pošli /api/state. Nově obsahuje probeIn, probeOut, largestAfterFirst a memoryGate: 0 = kontrola prošla, 1 = nedostatek celkové RAM, 2 = první TLS blok, 3 = druhý TLS blok. Kód 0 nezaručuje úspěšný TLS handshake.')
p('Větší půllitry, volitelný půlhodinový přípitek, priority letadel i nastavení zůstávají. Zdrojové zálohy a BIN RC4, RC3 a potvrzené RC2 jsou v ZIP. Fyzické ESP32 zatím nebylo pro RC5 otestováno.')
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
p('Kompilace ověřuje skutečné velikosti bufferů proti hlavičkám mbedTLS v Espressif core 3.3.0. Automatické testy zahrnují simulovanou roztříštěnou paměť, skutečný nedostatek prvního/druhého bloku, celkovou hranici 65 000 B, uvolnění zkušebních alokací, opakování po 15 sekundách a zachování posledních hodnot.')
p('Regresní testy letadel, měn, víkendového přechodu a webového rozhraní jsou přiložené. GitHub Actions provádí nezávislou kompilaci a prohlížečové testy s napodobeným ESP API; nejde o fyzický test na zařízení.')
p('Změna neupravuje partition table, NVS, Wi-Fi přihlašování ani uložené volby. Po nahrání ověř sítě, nastavení, načítání dat a odezvu webu/displeje. Pokud je stále paměti málo, nové diagnostické údaje rozliší konkrétní překážku.')
p('Světový čas není součástí RC5. Tato verze opravuje kontrolu HTTPS a zachovává funkce RC4 včetně větších půllitrů a přepínače přípitku.')
SimpleDocTemplate(str(r/'deliverables_3_2_rc5/RundeUhr_3_2_0_rc5_Navod_CZ.pdf'),rightMargin=48,leftMargin=48,topMargin=45,bottomMargin=45).build(story)
