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
h('RundeUhr 3.2.0-rc4')
p('Český návod • ESP32-C3 Super Mini • GC9A01 240 × 240 • 15. 11. 2026')
h('Větší půllitry a víkendový přípitek')
p('Oba půllitry na obrazovce Wochenende jsou přibližně 1,8× širší a vyšší. Zůstává pohyb přiťuknutí i původní odpočítávání před víkendem.')
p('Nová volba „Animovaný začátek víkendu – 30 minut“ nahrazuje půllitry postavičkou pouze během prvních 30 minut nastaveného víkendu. Má dlouhé vousy, vlasy stažené dozadu a černé tričko. Přípitek se opakuje ze čtyř kreslených poloh; nejde o plynulé video.')
p('Příklad: při začátku v pátek 16:00 se postavička zobrazuje od 16:00 do 16:29:59. V 16:30 se vrátí větší půllitry. Při kratším víkendu skončí přípitek už s jeho koncem. Restart časové okno neprodlužuje. Časové pásmo je Europe/Zurich.')
p('Přepínač je po aktualizaci vypnutý. Otevři Safari → Odpočítávání → Víkendový režim → zaškrtni Animovaný začátek víkendu – 30 minut → Uložit víkend. Musí být zapnuté také obecné animace. Při neplatném čase se přechod nespustí.')
p('Animace patří pouze obrazovce Wochenende. Běžné střídání obrazovek pokračuje a priorita letadel zůstává zachovaná. Pro souvislé sledování ručně vyber Wochenende a vypni automatické střídání; případná aktivní priorita letadel má nadále přednost.')
h('Nahrání z iPhonu přes Safari')
p('1. Stáhni samostatný RundeUhr_3_2_0_rc4.ino.bin do aplikace Soubory. ZIP není pro OTA potřeba.<br/>2. Připoj iPhone ke stejné Wi-Fi jako hodiny a otevři jejich místní IP adresu v Safari.<br/>3. Systém → Aktualizace firmware → vyber BIN → nahraj.<br/>4. Počkej na dokončení a restart. Ověř verzi 3.2.0-rc4 a nastavení víkendu.')
p('Aktualizace používá stejné NVS a OTA rozdělení jako RC3. Neobsahuje mazání Wi-Fi, nastavení nebo katalogů. Záloha RC3 a poslední uživatelem potvrzené RC2 je v ZIP. Tato verze ještě nebyla ověřena na skutečném ESP32.')
story.append(PageBreak())
h('Arduino IDE na Macu: přesné nastavení')
p('Pro běžnou aktualizaci používej OTA. Tento postup slouží pro kompilaci nebo obnovu přes USB. Otevři RundeUhr/RundeUhr.ino z úplného rozbaleného ZIP; všechny soubory ve složce musí zůstat pohromadě.')
p('Arduino IDE → Settings → Additional Boards Manager URLs:<br/><font size="8">https://espressif.github.io/arduino-esp32/package_esp32_index.json</font>')
p('Boards Manager: esp32 by Espressif Systems, verze 3.3.0.<br/>Library Manager: Adafruit GFX Library 1.12.1; Adafruit GC9A01A 1.1.0; Adafruit BusIO 1.17.2; ArduinoJson 7.4.2.')
rows=[['Tools','Hodnota'],['Board','ESP32C3 Dev Module'],['USB CDC On Boot','Enabled'],['CPU Frequency','160 MHz'],['Flash Size','4 MB (32 Mb)'],['Flash Mode / Frequency','DIO / 40 MHz'],['Partition Scheme','Minimal SPIFFS (1.9 MB APP with OTA)'],['Upload Speed','115200'],['Erase All Flash Before Sketch Upload','Disabled'],['Port','USB port připojeného ESP32-C3']]
t=Table([[Paragraph(a,style),Paragraph(b,style)] for a,b in rows],colWidths=[240,255]);t.setStyle(TableStyle([('VALIGN',(0,0),(-1,-1),'TOP'),('BACKGROUND',(0,0),(-1,0),HexColor('#e8edf4')),('LINEBELOW',(0,0),(-1,-1),.3,HexColor('#cad2dd'))]));story.append(t)
p('Soubor partitions.csv ve sketchi je závazný: dva OTA oddíly po 0x1E0000 = 1 966 080 B, NVS na 0x9000 o velikosti 0x5000. Neměň rozdělení ani nezapínej mazání Flash.')
p('Zapojení GC9A01: VCC → 3,3 V; GND → G; SCL → GPIO4; SDA → GPIO6; DC → GPIO5; CS → GPIO7; RST → GPIO3.')
p('BIN: 1 599 984 B. Rezerva OTA: 1 966 080 − 1 599 984 = 366 096 B. Statická RAM podle kompilátoru: 56 280 B; tato hodnota není měřením volné paměti během Wi-Fi/TLS.')
story.append(PageBreak())
h('Ověření a hranice testů')
p('Lokální kompilace pro ESP32-C3 prošla. Automatické testy ověřily časové hranice přechodu, vypnutí, neplatný čas, přechod týdne, kratší víkend, vykreslení fází a návrat barevné palety. Prošly také regresní testy letadel, měn, společného počasí a Slunce/Měsíce. Přiložené protokoly obsahují výsledky kontrol GitHub Actions.')
p('Na zařízení je potřeba ověřit: OTA z iPhonu, zachování sítí po restartu, viditelnost půllitrů, rychlost animace a ovládání Safari během načítání internetových dat. Fyzické zařízení nebylo pro tuto verzi dostupné.')
p('Pro rychlou zkoušku nastav začátek víkendu na dnešní den několik minut dopředu, zapni přechod a vyber Wochenende. Po zkoušce vrať své běžné časy. Animace zůstává omezená na první půlhodinu, i pokud přepínač zapneš později.')
p('Světový čas není součástí této verze; zůstává samostatným rozpracovaným požadavkem. RC4 se věnuje schválenému přechodu Wochenende a zvětšení půllitrů.')
SimpleDocTemplate(str(r/'deliverables_3_2_rc4/RundeUhr_3_2_0_rc4_Navod_CZ.pdf'),rightMargin=48,leftMargin=48,topMargin=45,bottomMargin=45).build(story)
