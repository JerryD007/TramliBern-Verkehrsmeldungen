# TramliBern Arduino-Kompiliertest – vorbereitet, noch nicht ausgeführt

Stand: 21.10.2026. Ziel: JerryD007/TramliBern-Verkehrsmeldungen.

Separater Testzweig: `test/arduino-esp32-compile`. Die GitHub-Verbindung wurde
repariert. Build und Speicherwerte werden durch GitHub Actions ermittelt.
Vorhandene Verkehrsmeldungs-Workflows und der Hauptzweig bleiben unverändert.

## Inhalt

- Hochgeladener Sketch unverändert unter `firmware/TramliBern/TramliBern.ino`.
- Eigener Workflow; vorhandene Verkehrsmeldungs-Workflows sind nicht Bestandteil des Pakets.
- Arduino CLI 1.5.1, ESP32 Core 3.3.2, ArduinoJson 7.4.2,
  GFX Library for Arduino 1.6.9 und XPT2046_Touchscreen 1.4 festgelegt.
- WiFi, HTTPClient, WebServer, Preferences, SPI und TLS kommen aus dem ESP32-Core.
- Testprofil: ESP32 Dev Module, 240 MHz, 4 MB Flash, Standardpartition, PSRAM aus.
  Dieses Profil ist eine Testannahme; die aktuellen Arduino-IDE-Einstellungen wurden nicht vorgelegt.

## Einrichten nach Freigabe des GitHub-Schreibzugriffs

1. Vom Hauptzweig einen separaten Zweig `test/arduino-esp32-compile` erstellen.
2. Die drei Verzeichnisse `.github`, `firmware` und `ci` sowie diese Dokumentation
   auf diesen Testzweig übernehmen; keine vorhandenen Dateien ersetzen.
3. Push startet den neuen Workflow. Ergebnisse stehen unter Actions im Lauf und
   im Artefakt `arduino-esp32-report` (30 Tage Aufbewahrung).
4. Nur einen Prüf-PR öffnen, nicht zusammenführen und kein Auto-Merge aktivieren.

Weitere Änderungen an den relevanten Dateien im Testzweig sowie passende Pull Requests
starten neue Tests. Manuelles workflow_dispatch ist vorgesehen; der GitHub-UI-Button
kann fehlen, solange der Workflow nicht auf dem Standardzweig liegt. Push genügt.

## Lokal wiederholen

Arduino CLI 1.5.1 installieren, dann im Paketverzeichnis:

```bash
bash ci/install.sh
bash ci/compile.sh
```

`reports/memory.md` dokumentiert Programmplatz und statischen RAM mit Bytewerten,
Grenzwerten, Restplatz und Prozentberechnung. Ein fehlender Speicherbericht lässt
einen ansonsten erfolgreichen CI-Lauf fehlschlagen. Compilerfehler bleiben Fehler.
Laufzeitverbrauch durch Heap/Stack, HTTPS und JSON ist damit nicht gemessen.
Der Test lädt keine Firmware auf das Board und veröffentlicht keine Verkehrsdaten.
