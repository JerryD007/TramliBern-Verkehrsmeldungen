from pathlib import Path
import json
from reportlab.platypus import SimpleDocTemplate,Paragraph,Spacer,PageBreak,Table,TableStyle
from reportlab.lib.styles import ParagraphStyle
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.lib.colors import HexColor
r=Path(__file__).resolve().parents[2];out=r/'deliverables_rc17'
pdfmetrics.registerFont(TTFont('DV','/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'));pdfmetrics.registerFont(TTFont('DVB','/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf'))
s=ParagraphStyle('body',fontName='DV',fontSize=10,leading=15,spaceAfter=10);h=ParagraphStyle('h',fontName='DVB',fontSize=18,leading=23,spaceAfter=14,textColor=HexColor('#244369'));story=[]
def p(t):story.append(Paragraph(t,s))
def title(t):story.append(Paragraph(t,h))
title('RundeUhr RC17 - ISS Radar')
p('Český návod a plán rozšíření • ESP32-C3 Super Mini, GC9A01 240 × 240 • 16. 11. 2026')
p('RC17 přidává samostatnou obrazovku ISS Radar. Výchozí cyklus se nemění. V Safari otevři Obrazovky → ISS Radar → Vybrat, případně zapni V cyklu. Polohu pozorovatele nastavíš v Systém → Poloha pro počasí, slunce a ISS. Není třeba účet, senzor ani změna zapojení.')
p('Radar: N je sever, střed je zenit a kružnice představuje obzor. ISS pod obzorem se zobrazí na okraji ve správném azimutu. Číslo km je přímá vzdálenost, Az azimut a El přibližná výška nad obzorem ve stupních. Model používá kulovou Zemi bez nadmořské výšky pozorovatele; nejde o přesný astronomický výpočet. Nad obzorem neznamená viditelnost očima. Předpověď dalšího přeletu není v RC17.')
p('Zdroj wheretheiss.at se načítá po 30 sekundách, pouze pokud je ISS vybraná nebo v cyklu. Po chybě čeká 5 minut; údaj starší než 90 sekund je označen jako neaktuální. Při výpadku zůstane poslední údaj viditelný šedě. Čas polohy a chyba jsou také v diagnostice. Žádné údaje nejsou dopočítávány jako údajně živé.')
title('Nahrání přes Safari')
for t in ['1. Před nahráním exportuj vlastní nastavení v místní aplikaci. Záloha v ZIP obsahuje RC16, ne tvoje soukromé nastavení.','2. Stáhni samostatný RundeUhr_3_2_0_rc17.ino.bin. ZIP na iPhonu pro OTA nerozbaluj.','3. Připoj iPhone ke stejné Wi-Fi jako hodiny, otevři místní aplikaci a nahraj BIN v aktualizaci firmware. Vyčkej na restart.','4. Ověř verzi 3.2.0-rc17 a novou kartu ISS Radar. Pokud zůstala stará stránka, jednou ji obnov.','5. Ověř uložené Wi-Fi, původní obrazovky, prioritu letadel a reakci webu při načítání ISS. Skutečnou funkci TLS/RAM a OTA na tvých hodinách musí ověřit tento fyzický test.']:p(t)
p('<b>Návrat k RC16:</b> nejdříve vypni ISS v cyklu, vyber původní obrazovku (např. hodiny) a nech alespoň 3 sekundy uložit změnu. Potom nahraj RC16 ze zálohy. Starší firmware nezná ID nové obrazovky; bez tohoto kroku nemusí přijmout novou konfiguraci.')
story.append(PageBreak());title('Arduino IDE a ověření')
p('Boards Manager URL:<br/><font size="8">https://espressif.github.io/arduino-esp32/package_esp32_index.json</font>')
p('Boards Manager: esp32 by Espressif Systems 3.3.0. Library Manager: Adafruit GFX Library 1.12.1; Adafruit GC9A01A 1.1.0; Adafruit BusIO 1.17.2; ArduinoJson 7.4.2.')
rows=[['Tools','Hodnota'],['Board','ESP32C3 Dev Module'],['USB CDC On Boot','Enabled'],['CPU Frequency','160 MHz'],['Flash Size','4 MB (32 Mb)'],['Flash Mode / Frequency','DIO / 40 MHz'],['Partition Scheme','Minimal SPIFFS (1.9 MB APP with OTA)'],['Upload Speed','115200'],['Erase All Flash Before Sketch Upload','Disabled'],['Port','USB port tvého ESP32-C3']]
t=Table([[Paragraph(a,s),Paragraph(b,s)] for a,b in rows],colWidths=[230,265]);t.setStyle(TableStyle([('VALIGN',(0,0),(-1,-1),'TOP'),('BACKGROUND',(0,0),(-1,0),HexColor('#e8edf4')),('LINEBELOW',(0,0),(-1,-1),.3,HexColor('#cad2dd'))]));story.append(t)
p('Otevři celý sketch zdroje/RundeUhrWeb/RundeUhr/RundeUhr.ino včetně všech souborů. Přiložený partitions.csv je rozhodující: 2 × 1 966 080 B OTA, NVS 0x9000 / 0x5000. Nemazat celou Flash.')
p('Zapojení: VCC → 3,3 V; GND → G; SCL → GPIO4; SDA → GPIO6; DC → GPIO5; CS → GPIO7; RST → GPIO3.')
b=out/'RundeUhr_3_2_0_rc17.ino.bin'
if b.exists():p(f'BIN má {b.stat().st_size} B; rezerva OTA = 1 966 080 − {b.stat().st_size} = {1966080-b.stat().st_size} B.')
p('Výsledky kompilace a automatických testů jsou přiloženy v ZIP. Testy webu používají WebKit a Chromium s napodobeným API ESP32. Místní testy ověřují chybějící/staré/budoucí údaje, geometrii ISS a hranice kulatého displeje. Živé HTTP testy probíhaly na počítači/CI, nikoli na hodinách. Statická RAM z kompilace neudává volnou RAM při HTTPS.')
p('RC17 nebyla v tomto prostředí otestována na fyzickém ESP32 ani vzdáleně nahrána. Hlavní větev zůstala nezměněná. Zdrojové soubory správy Wi-Fi, ukládání nastavení a rozdělení Flash zůstávají zachované.')
story.append(PageBreak());title('Další RC: pořadí a podmínky')
p('Hotová první etapa je ISS. Následující položky jsou plánované, nikoli již zapracované. Čísla RC mohou změnit opravná vydání po testu na hodinách.')
for t in ['<b>RC18 - zemětřesení:</b> USGS GeoJSON M4.5+ za posledních 24 hodin, předběžně obnova po 5 minutách. Bez účtu nebo senzoru. Zobrazení zveřejněných událostí; není to předpověď. Malá švýcarská zemětřesení tento feed nemusí pokrývat.','<b>RC19 - starty raket:</b> Launch Library 2 v2.3.0, bez účtu, limit 15 požadavků za hodinu. Vývojové API test prošel, ale jeho data jsou záměrně zastaralá. Počet hodin sdílejících IP musí určovat interval/caching. NET je plánovaný nejdřívější čas; odklad nesmí být označen jako skutečný start.','<b>RC20 - německé zprávy:</b> oficiální RSS tagesschau pro soukromé nekomerční čtení. Titulek, zdroj, čas a odkaz; bez publikování článků. Nutný streamový XML parser, testy entit, CDATA, UTF-8, dlouhých titulků a neúplné odpovědi.','<b>RC21 - Bambu Lab X1 Carbon:</b> místní monitor průběhu, zbývajícího času a teplot. IP, sériové číslo a LAN access code zadat pouze do místní aplikace. Ještě ověřit firmware tiskárny a čtení stavu v jejím současném režimu. Nejde o garantované stabilní veřejné API. Trvalé MQTT/TLS vyžaduje zátěžový test RAM; v první etapě bez kamery a ovládacích příkazů.']:p(t)
p('Živé testy: ISS 312 B, USGS 8 020 B, testovací rakety 1 272 B, RSS 55 903 B při prvním měření. Velikosti se mění. Všechny čtyři zdroje vrátily HTTP 200 a následný test struktury prošel. Tiskárna nebyla dostupná pro fyzický test. Před přidáním více než 16 interních obrazovek bude nutné rozšířit masku cyklu.')
p('Nový ISS kód je vlastní implementace; kód cizích projektů se nepřebírá. U BambuHelper nebyla metadaty GitHub potvrzena licence, proto nelze převzít jeho kód bez dalšího ověření oprávnění.')
story.append(PageBreak());title('Primární zdroje')
for name,url in [('ISS API, limity a formát','https://docs.wheretheiss.at/w/developer'),('USGS strojový GeoJSON feed','https://earthquake.usgs.gov/earthquakes/feed/v1.0/geojson.php'),('USGS použití dat a uvedení zdroje','https://pubs.usgs.gov/documentation/faq'),('Launch Library 2 a limit API','https://ll.thespacedevs.com/docs'),('The Space Devs podmínky dat','https://github.com/TheSpaceDevs/Tutorials/blob/main/faqs/faq_TSD.md'),('Tagesschau RSS a soukromé použití','https://www.tagesschau.de/infoservices/rssfeeds'),('Bambu Lab integrace třetích stran','https://blog.bambulab.com/updates-and-third-party-integration-with-bambu-connect/'),('BambuHelper, existující realizace','https://github.com/Keralots/BambuHelper')]:p(name+'<br/><font size="8"><link href="'+url+'">'+url+'</link></font>')
p('Ověřeno 16. 11. 2026. Podmínky a API mohou být později změněny; ověřit je znovu při implementaci příslušné etapy.')
SimpleDocTemplate(str(out/'RundeUhr_3_2_0_rc17_Navod_CZ.pdf'),leftMargin=48,rightMargin=48,topMargin=42,bottomMargin=42).build(story)
