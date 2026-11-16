from pathlib import Path
from reportlab.platypus import SimpleDocTemplate,Paragraph,Spacer,PageBreak,Table,TableStyle,Image
from reportlab.lib.styles import ParagraphStyle
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.lib.colors import HexColor
import hashlib
r=Path(__file__).resolve().parents[2];out=r/'deliverables_rc15';b=out/'RundeUhr_3_2_0_rc15.ino.bin'
pdfmetrics.registerFont(TTFont('DV','/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'));pdfmetrics.registerFont(TTFont('DVB','/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf'))
s=ParagraphStyle('body',fontName='DV',fontSize=10,leading=15,spaceAfter=10);h=ParagraphStyle('title',fontName='DVB',fontSize=18,leading=23,spaceAfter=14,textColor=HexColor('#244369'));story=[]
def p(t):story.append(Paragraph(t,s))
def title(t):story.append(Paragraph(t,h))
title('RundeUhr 3.2.0-rc15')
p('Český návod • ESP32-C3 Super Mini • GC9A01 240 × 240 • 16. 11. 2026')
title('Schválené změny vzhledu')
p('Letadlo: kompaktní žlutá ikona s kokpitem, dvěma motory, křídly a ocasními plochami. Stejná ikona je použitá s dostupnou trasou i bez ní. Větší boční údaje výšky a rychlosti. Vrtulník zůstává červený. Názvy míst VON / NACH jsou zachovány.')
p('Wetter & Regen: větší text dvou spodních hodinových předpovědí a větší malé ikonky. Neobvykle dlouhé hodnoty používají menší písmo, aby zůstaly celé a nepřekrývaly sousední údaj.')
p('Geburtstag: fialový dárek s tyrkysovou mašlí a konfetami, fialový okraj a odpočet. Animace respektuje stávající přepínač animací.')
p('Jde o vykreslovací kód pro 240 × 240 bodů. Hrany a drobné detaily budou na skutečném displeji pixelové; obrazové návrhy byly ilustrace. Feuerwehr není do této aktualizace zařazeno.')
title('Nahrání z iPhonu přes Safari')
for t in ['1. Stáhni samostatný RundeUhr_3_2_0_rc15.ino.bin do aplikace Soubory. ZIP nerozbaluj pro běžnou OTA aktualizaci.','2. Připoj iPhone ke stejné místní Wi-Fi jako hodiny a otevři jejich místní adresu v Safari.','3. V místní webové aplikaci otevři aktualizaci firmware, vyber tento .ino.bin a spusť nahrávání. Používej stabilní napájení.','4. Vyčkej na dokončení a restart. V diagnostice ověř verzi 3.2.0-rc15. Zkontroluj počasí, narozeniny a letadlo s trasou i bez trasy.','5. Zkontroluj zachování sítí a nastavení. Při problému lze přes stejné OTA rozhraní nahrát původní RC14 z přiložené zálohy.'] :p(t)
p('Firmware nebyl vzdáleně nahrán do zařízení. Uživatelská data nejsou součástí zdrojové zálohy; vlastní zálohu nastavení lze vytvořit v aplikaci.')
story.append(PageBreak());title('Arduino IDE na Macu')
p('Tato část slouží pro vlastní kompilaci nebo obnovu přes USB. Otevři celý sketch RundeUhr/RundeUhr.ino ze složky zdroje/RundeUhrWeb. Zachovej všechny soubory sketchu.')
p('Settings → Additional Boards Manager URLs:<br/><font size="8">https://espressif.github.io/arduino-esp32/package_esp32_index.json</font>')
p('Boards Manager: esp32 by Espressif Systems 3.3.0.<br/>Library Manager: Adafruit GFX Library 1.12.1; Adafruit GC9A01A 1.1.0; Adafruit BusIO 1.17.2; ArduinoJson 7.4.2.')
rows=[['Tools','Hodnota'],['Board','ESP32C3 Dev Module'],['USB CDC On Boot','Enabled'],['CPU Frequency','160 MHz'],['Flash Size','4 MB (32 Mb)'],['Flash Mode / Frequency','DIO / 40 MHz'],['Partition Scheme','Minimal SPIFFS (1.9 MB APP with OTA)'],['Upload Speed','115200'],['Erase All Flash Before Sketch Upload','Disabled'],['Port','USB port připojeného ESP32-C3']]
t=Table([[Paragraph(a,s),Paragraph(b,s)] for a,b in rows],colWidths=[230,265]);t.setStyle(TableStyle([('VALIGN',(0,0),(-1,-1),'TOP'),('BACKGROUND',(0,0),(-1,0),HexColor('#e8edf4')),('LINEBELOW',(0,0),(-1,-1),.3,HexColor('#cad2dd'))]));story.append(t)
p('Rozhodující je přiložený partitions.csv: dva OTA oddíly po 1 966 080 B; NVS 0x9000 / 0x5000. Rozdělení se proti RC14 nemění. Nepoužívej mazání celé Flash.')
p('Zapojení: VCC → 3,3 V; GND → G; SCL → GPIO4; SDA → GPIO6; DC → GPIO5; CS → GPIO7; RST → GPIO3.')
if b.exists():
 n=b.stat().st_size;p(f'Zkompilovaný BIN: {n:,} B. Rezerva v OTA oddílu: 1 966 080 − {n:,} = {1966080-n:,} B.')
story.append(PageBreak());title('Testy a jejich omezení')
p('Místní testy vykreslení: trasa s oběma či jedním koncem, bez trasy, neznámý typ, chybějící telemetrie, offline stav, krajní hodnoty, hranice kruhu, větší počasí a animace narozenin. Spodní část trasy byla porovnána po pixelech s původním vykreslením.')
p('Kontrola zdrojů: schéma a ukládání nastavení, správa více Wi-Fi, OTA, rozdělení Flash a síťové načítání se proti RC14 nezměnily. Nejde o fyzický test zachování dat na tvých hodinách.')
p('GitHub Actions: výsledky a protokoly kompilace a automatických testů jsou v balíčku. Testy prohlížečů pracují s napodobeným API ESP32. Kontrola BIN ověřuje ESP32-C3, rodinu OTA, SHA-256 a velikost oproti OTA oddílu.')
p('Test na skutečném zařízení v tomto prostředí nebyl proveden. Ověření rychlosti displeje, skutečné volné RAM při TLS, Wi-Fi a nahrání přes Safari vyžaduje tvoje ESP32. Statická RAM z kompilátoru není měřením volné RAM za provozu.')
p('Záloha v ZIP obsahuje původní kompletní balíček RC14. Hlavní větev projektu ani firmware v hodinách nebyly změněny. Nová verze je připravena v samostatné testovací větvi.')
SimpleDocTemplate(str(out/'RundeUhr_3_2_0_rc15_Navod_CZ.pdf'),leftMargin=48,rightMargin=48,topMargin=45,bottomMargin=45).build(story)
