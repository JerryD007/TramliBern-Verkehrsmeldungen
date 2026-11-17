from pathlib import Path
import json
from reportlab.platypus import SimpleDocTemplate,Paragraph,PageBreak,Table,TableStyle
from reportlab.lib.styles import ParagraphStyle
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.lib.colors import HexColor
r=Path(__file__).resolve().parents[2];out=r/'deliverables_rc25';out.mkdir(exist_ok=True)
for name,file in [('DV','DejaVuSans.ttf'),('DVB','DejaVuSans-Bold.ttf')]:pdfmetrics.registerFont(TTFont(name,'/usr/share/fonts/truetype/dejavu/'+file))
s=ParagraphStyle('body',fontName='DV',fontSize=10,leading=14,spaceAfter=9)
h=ParagraphStyle('h',fontName='DVB',fontSize=17,leading=22,spaceAfter=13,textColor=HexColor('#244369'));story=[]
def p(t):story.append(Paragraph(t,s))
def title(t):story.append(Paragraph(t,h))
title('RundeUhr RC25 – ISS, rakety a nové zprávy')
p('Český návod • ESP32-C3 Super Mini • GC9A01 240 × 240 • 17. 11. 2026')
p('Verze vychází z RC24. Zachovává Wi-Fi, uživatelská nastavení, OTA oddíly, obrazovky a jejich design. Záloha RC24 je v kompletním ZIP.')
p('<b>Rakety:</b> maximum časového okna je 10 080 minut (7 dní). Pro dva dny nastav 2 880, pro tři dny 4 320. Nastavení je v Einstellungen → Aktuelle Ereignisse im Wechsel → Start innerhalb der nächsten Minuten. Potvrď Speichern. Dosavadní hodnota zůstává zachovaná, nezvětšuje se automaticky.')
p('Stále platí filtr: načtená aktuální data, stav Go, termín přesný na minutu a start v zadaném okně. Rozšíření limitu nezaručuje zobrazení nepotvrzených startů ani termínů s přesností jen na hodinu. Ruční zobrazení zůstává možné.')
p('<b>ISS:</b> System → Detaillierte Diagnose obsahuje ISS letzter Versuch, ISS TLS-Detail, ISS letzter TLS-Fehler a ISS RAM / grösster Block vor TLS. Záznam zachytí ověřovací příznaky před uvolněním TLS spojení. Poslední TLS chyba a její čas zůstávají dostupné i po dalším úspěšném spojení; restart tento záznam vymaže.')
p('Kontrola certifikátu zůstává povinná. Neúspěšné ověření uzavře spojení před odesláním HTTP požadavku. U obecné chyby TCP nebo timeoutu nemusí být konkrétní příčina certifikátu dostupná. Záznam neobsahuje hesla k Wi-Fi.')
p('<b>Důležité:</b> RC25 doplňuje diagnostiku, sama o sobě nepotvrzuje odstranění občasných chyb TLS u ISS. Načítání zemětřesení se touto verzí nemění. Označení webu bylo opraveno z pevně zapsané RC22 na RC25.')
p('<b>Zprávy:</b> v Nachrichten → Sender / Region přibyly ČT24 · Česko a svět (CZ) a CNN International · via Google News (EN). Vyber zdroj a Quelle speichern. Titulky zůstávají v původním jazyce. CNN je přes externí agregátor Google News, ne přímý RSS CNN; starý CNN RSS při kontrole vracel zprávy z roku 2023. ČT24 používá přímý RSS výběru redakce. Obě nové služby byly ověřeny živým testem na počítači.')
p('Česká písmena s háčky a čárkami mají vlastní malé bitmapové znaky, bez velkého fontového bufferu. Ukládají se tři nejnovější titulky. Interval načítání zůstává 90 minut; filtr stáří zpráv může starší titulky v cyklu vynechat. CNN odkazy v API vedou na domovskou stránku vydavatele; dlouhé přesměrovací URL se do RAM neukládají.')
story.append(PageBreak());title('Nahrání přes Safari')
for t in ['1. Stáhni samostatný <b>RundeUhr_3_2_0_rc25.ino.bin</b>. Na iPhonu nemusíš rozbalovat ZIP.','2. Na stejné Wi-Fi otevři IP hodin v Safari. V System → Einstellungssicherung ulož zálohu nastavení. Ta neobsahuje Wi-Fi hesla.','3. System → Firmware-Update: vyber BIN a nahraj ji. Neodpojuj napájení a nech Safari otevřené.','4. Po restartu ověř verzi <b>3.2.0-rc25</b> a obnov web. Nastavení, uložené Wi-Fi a původní obrazovky zůstávají.','5. Pro návrat je v ZIP úplná záloha RC24 včetně BIN. Nahrává se stejným postupem.']:p(t)
story.append(PageBreak());title('Kompilace a nastavení Arduino IDE')
p('Boards Manager URL:<br/><font size="8">https://espressif.github.io/arduino-esp32/package_esp32_index.json</font>')
p('Boards Manager: <b>esp32 by Espressif Systems 3.3.0</b>.<br/>Library Manager: <b>Adafruit GFX Library 1.12.1</b>, <b>Adafruit GC9A01A 1.1.0</b>, <b>Adafruit BusIO 1.17.2</b>, <b>ArduinoJson 7.4.2</b>.')
rows=[['Tools','Hodnota'],['Board','ESP32C3 Dev Module'],['USB CDC On Boot','Enabled'],['CPU Frequency','160 MHz'],['Flash Size','4 MB (32 Mb)'],['Flash Mode / Frequency','DIO / 40 MHz'],['Partition Scheme','Minimal SPIFFS (1.9 MB APP with OTA)'],['Upload Speed','115200'],['Erase All Flash Before Sketch Upload','Disabled'],['Port','USB port připojeného ESP32-C3']]
t=Table([[Paragraph(a,s),Paragraph(b,s)] for a,b in rows],colWidths=[225,270]);t.setStyle(TableStyle([('VALIGN',(0,0),(-1,-1),'TOP'),('BACKGROUND',(0,0),(-1,0),HexColor('#e8edf4')),('LINEBELOW',(0,0),(-1,-1),.3,HexColor('#cad2dd'))]));t.spaceAfter=10;story.append(t)
p('Otevři celý sketch <b>zdroje/RundeUhrWeb/RundeUhr/RundeUhr.ino</b>. Přiložený partitions.csv zachovává dvě OTA oblasti po 1 966 080 B a NVS na 0x9000 o délce 0x5000. Nemazat celou Flash. Pro USB nahrání použij Upload; při běžné aktualizaci stačí Safari.')
p('Zapojení: VCC → 3,3 V; GND → G; SCL → GPIO4; SDA → GPIO6; DC → GPIO5; CS → GPIO7; RST → GPIO3.')
b=out/'RundeUhr_3_2_0_rc25.ino.bin'
if b.exists():p(f'BIN: <b>{b.stat().st_size:,} B</b>. Rezerva OTA = 1 966 080 - {b.stat().st_size:,} = <b>{1966080-b.stat().st_size:,} B</b>.'.replace(',',' '))
report=out/'Vysledky_testu_RC25.json'
if report.exists():
 d=json.loads(report.read_text());p(f'Statická RAM RC25: <b>{d["static_ram"]} B</b> (RC24: 62 216 B). To není měření volné RAM při HTTPS.')
p('Úplný záznam kompilace, testů a SHA-256 je v ZIP. Zásobníky hlavní i síťové úlohy mají nadále 12 KiB. Uložená Wi-Fi a nastavení se zachovávají. TLS ověřování certifikátů zůstává zapnuté.')
story.append(PageBreak());title('Ověření a hranice testů')
p('<b>Počítač / GitHub Actions:</b> skutečná kompilace ESP32-C3, kontrola BIN a OTA oddílů, parsování živých HTTPS odpovědí, testy původních funkcí, ukládání konfigurace, paměťových bloků a časovačů. WebKit a Chromium používají simulované API ESP32. Grafické testy kontrolují kruhové hranice; náhledy byly také vizuálně prohlédnuty.')
p('Nové hostitelské testy simulují chybu certifikátu, ověřovací příznaky, uzavření neúspěšného spojení, úspěšné ověření a uchování poslední chyby. Test časového okna ověřuje hranici 10 080 minut a odmítnutí větší hodnoty. Jde o počítačové testy, nikoli skutečný TLS handshake na ESP32.')
p('<b>RC25 nebyla testována na fyzickém ESP32.</b> Živé HTTPS testy na počítači nenahrazují ověření certifikátu a paměťových špiček na hodinách. Zařízení zůstává nutné pro potvrzení příčiny přerušovaných výpadků.')
title('Kontrola po nahrání')
for t in ['Nahraj nejprve jedny hodiny, obnov Safari a v System ověř firmware i Oberfläche 3.2.0-rc25.', 'U raket zadej například 2880 a Speichern. Po obnovení stránky ověř uloženou hodnotu. Im Wechsel musí být zapnuté.', 'Nech ISS zapnutou v cyklu několik minut. Pokud nastane chyba, pořiď snímek nových řádků ISS TLS-Detail a ISS letzter TLS-Fehler v Detaillierte Diagnose. Před snímkem nerestartuj hodiny.', 'I při úspěšném posledním požadavku zůstane předchozí TLS chyba viditelná s časem. Aktuell označuje čerstvost uložených dat, nikoli historii bez chyb.', 'Ověř zachování Wi-Fi, nastavení a původních obrazovek. Záložní RC24 je v ZIP; při návratu z RC25 s oknem raket nad 360 nejdřív nastav nejvýše 360 a ulož, a vyber některý původní zdroj zpráv (SRF nebo tagesschau), aby RC24 přijala konfiguraci.']:p(t)
def footer(c,d):c.setFont('DV',8);c.setFillColor(HexColor('#627389'));c.drawString(42,25,'RundeUhr 3.2.0-rc25 • Český návod');c.drawRightString(553,25,str(d.page))
SimpleDocTemplate(str(out/'RundeUhr_3_2_0_rc25_Navod_CZ.pdf'),pagesize=(595,842),leftMargin=42,rightMargin=42,topMargin=38,bottomMargin=42).build(story,onFirstPage=footer,onLaterPages=footer)
