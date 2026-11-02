# TramliBern SLIM – český návod (testovací verze, 2. 11. 2026)

## Co je v programu

- ESP32 + 4" ST7796, dotyk XPT2046, displej 480 × 320 na šířku.
- Živé odjezdy v několika švýcarských městech; modré tlačítko města a červené ABFAHRTEN.
- Tři nezávisle nastavitelné stránky: strana 1 a 2 mají dvě zastávky po třech odjezdech; strana 3 jednu zastávku se sedmi odjezdy. Mohou zůstat prázdné.
- Správa zastávek dotykem a přes lokální webovou aplikaci v Safari, včetně více zařízení.
- Spodní tlačítko MENU a tlačítka stránek 1, 2, 3.
- Diagnostika odjezdů v Safari pod /diagnose: stav Wi-Fi, stáří posledních úspěšných dat a případná chyba každé zastávky. Údaj „naposledy úspěšně“ se počítá od posledního restartu.
- Až čtyři uložené Wi-Fi sítě a výběr uložené sítě pomocí MENU → WLAN. Novou síť a heslo je zatím potřeba zadat přes webovou aplikaci (Nastavení).
- Bez dopravních výluk, GTFS alarmů a SLOID diagnostiky.

## Co zatím není dokončeno

- Ruční regulace podsvícení, noční režim a upozornění na nízkou baterii vyžadují další otestování.
- Zadání nového Wi-Fi hesla přímo na displeji není v této verzi.
- Kompilace v cloudu nenahrazuje zkoušku dotykového displeje, baterie ani přepínání sítí na reálné desce.

## Bezpečná záloha

V balíčku jsou dvě **různé** verze:
- `TramliBern/TramliBern.ino`: nové testovací vydání s diagnostikou, MENU a více Wi-Fi.
- `Zaloha_pred_upravami/TramliBern/TramliBern.ino`: přesná GitHub kopie větve `backup/tramlibern-slim-free-pages-20261102`, před těmito úpravami.

Záloha v balíčku **není automaticky potvrzený obraz firmware právě nahraného ve tvém ESP32**. Pokud chceš zálohovat přesně dosavadní fungující firmware, uschovej také jeho původní soubor .ino na Macu.

## Postup na Macu v Safari a Arduino IDE

1. Z GitHub Actions stáhni artefakt `TramliBern-SLIM-CZ-balik` a rozbal ZIP.
2. Ulož si celý archiv jako bezpečnostní kopii.
3. Otevři soubor `TramliBern/TramliBern.ino` v Arduino IDE. Složka a soubor mají stejný název.
4. Pokud Arduino oznámí, že složka `TramliBern` už existuje, **nic nepřepisuj**. Zkopíruj celý adresář pod názvem `TramliBern_SLIM_TEST` a uvnitř přejmenuj soubor na `TramliBern_SLIM_TEST.ino`.
5. Zvol stejnou desku a USB port jako dosud: pro GitHub test byl použit `ESP32 Dev Module`, 4 MB Flash, standardní rozdělení paměti, PSRAM vypnutá.
6. Potřebné knihovny: `GFX Library for Arduino`, `ArduinoJson`, `XPT2046_Touchscreen`. GitHub používá ESP32 core 3.3.2.
7. Klikni na ✓ **Ověřit**. Až po úspěšné kompilaci můžeš kliknout na → **Nahrát**.
8. Po nahrání vyzkoušej všechny stránky, změnu města, MENU, dotyk, Safari a zastávky. V Safari otevři IP adresu z displeje a vyber **Abfahrtsdiagnose ansehen**.
9. Připojení: pokud žádná uložená síť není dostupná, zařízení poskytuje vlastní Wi-Fi `TRAMLI-BERN-2` a web `http://192.168.4.1`. Novou síť ulož přes Nastavení.

## Zkouška více Wi-Fi

Ulož druhou Wi-Fi síť přes Nastavení. Po restartu vypni dostupnost první sítě a ověř, že zařízení najde druhou. Zkontroluj také volbu MENU → WLAN. Nikdy nezveřejňuj hesla ze svých sítí.

## Diagnostika

- **WLAN Verbunden**: ESP32 je připojený.
- **Noch keine erfolgreichen Daten**: od posledního restartu se pro zastávku ještě nenačetly žádné odjezdy.
- **Letztes erfolgreiches Laden**: poslední úspěšné stažení dat.
- **Alter der gespeicherten Daten**: čas od stažení, nikoli zaručené stáří samotného jízdního řádu u dopravce.
- **Fehler**: zjednodušené vysvětlení problému. Při chybě zařízení opakuje načtení automaticky.

## Co jsme ověřili

Automatický GitHub Actions test aktuálního zdrojového kódu prošel 2. listopadu 2026. Obsahuje kompilaci firmware pro ESP32 a doplňkové Python testy. Tyto doplňkové testy se ještě týkají starší GTFS infrastruktury a neověřují hardware ani kompletní chování nového MENU či Wi-Fi.

Repozitář: https://github.com/JerryD007/TramliBern-Verkehrsmeldungen
Testovací větev: `test/tramlibern-slim-diagnostics`
Záložní větev: `backup/tramlibern-slim-free-pages-20261102`
