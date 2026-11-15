FEUERWEHR - vývojový prototyp, 15. 11. 2026

Ověření zdroje
Přímo na https://myfeuerwehr.ch/information/legal byl přečten bod 1. Podmínky požadují předchozí písemný souhlas provozovatele k reprodukci, přenosu, úpravě, propojení nebo použití webu pro soukromé, veřejné i komerční účely. Samostatně připouštějí stažení či tisk jednotlivých stránek se zachováním autorských označení. Veřejná dostupnost stránky tedy sama neposkytuje souhlas k automatickému přebírání pro tento projekt.
Dokumentované a podporované veřejné API pro alarmy se nepodařilo dohledat. Nelze potvrdit, že takové API existuje. Nejde o důkaz jeho neexistence. Provozovatelem uvedeným v podmínkách je Gebäudeversicherung Bern (GVB).

Hotová část
Samostatný renderer 240 × 240 px: FEUERWEHR, vlastní ikona vozu, obec, běžící německý text, čas a informace o zpoždění. Dlouhý text se posouvá rychlostí 20 px/s s dvousekundovou pauzou na začátku a konci. Krátký text stojí. Ukázka je výslovně označena DEMO; obec, zpráva a čas představují smyšlený příklad, nikoli skutečný zásah. Prodleva v ukázce je také pouze ukázkový údaj.

Ověření a omezení
Hostitelský C++ test s Adafruit GFX 1.12.1 prošel: posun, pauzy, krátký text, ořezání pásu, zachování statických údajů a rozlišení stavů bez zdroje, bez zpráv a při chybě. Test není testem na skutečném ESP32. Renderer není integrován do nabídky firmware a neobsahuje síťové načítání. Nejde o novou instalovatelnou verzi; nový soubor .ino.bin se nevydává. Funkční RC14 a nastavení zařízení nebyly změněny.

Obsah ZIP
Zdrojový základ RC14, experimentální renderer a testy, výsledek místního testu, animovaná ukázka a kompletní původní balíček RC14 jako záloha. Záloha obsahuje jeho původní firmware a český návod s nastavením Arduino IDE. Prototyp se do hodin nenahrává. Spuštění počítačového testu: nastavte GFX_DIR na adresář Adafruit GFX 1.12.1 a spusťte bash experiments/feuerwehr/run.sh v kořeni zdrojů. Potřebný je C++17 kompilátor.

Další podmínka připojení dat
Získat písemný souhlas a specifikaci rozhraní, intervalů, limitů, povinného uvedení zdroje a povolených polí. Teprve podle odpovědi lze dokončit načítání, ovládání v Safari a ověřit Flash, RAM, OTA i chování při výpadku. Žádost níže nebyla odeslána.

Betreff: Private Anzeige öffentlicher Feuerwehralarmierungen auf einem ESP32

Guten Tag

ich möchte öffentlich zugängliche Feuerwehrmeldungen aus dem Kanton Bern auf einem privaten ESP32-Display zu Hause anzeigen. Geplant sind Gemeinde, öffentlich freigegebene Ereigniskategorie und Zeitpunkt, unter Beibehaltung einer von Ihnen vorgegebenen Verzögerung. Nicht öffentliche Einsatzdaten sollen weder abgerufen noch angezeigt werden. Eine öffentliche Weiterverbreitung ist nicht vorgesehen.

Erteilen Sie dafür eine schriftliche Zustimmung gemäss Ziffer 1 Ihrer rechtlichen Hinweise? Gibt es eine offiziell unterstützte API oder einen Feed? Bitte teilen Sie mir Dokumentation, erlaubte Abrufintervalle, Nutzungslimits, erforderliche Quellenangaben und weitere Bedingungen mit. Falls eine Kategorie nicht öffentlich freigegeben ist, würde die Anzeige entsprechend eingeschränkt.

Vor Ihrer Zustimmung wird keine automatische Übernahme eingerichtet.

Vielen Dank und freundliche Grüsse
Jaroslav
