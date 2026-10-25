# TramliBern 3.2 TEST – Download und sicherer Gerätetest

Diese Testversion ist kompiliert. Ein erfolgreicher Build ersetzt keinen Gerätetest.
Die verifizierte Warnversorgung ist zunächst auf die fünf dokumentierten Berner
Haltestellen begrenzt. Stadtwechsel, drei Seiten, Tastatur, Haltestellenverwaltung,
Live-Abfahrten und Web-App/Multi-Board bleiben im Sketch erhalten.

## Download

Im erfolgreichen Workflow **TramliBern 3.2 verification and firmware** das Artefakt
`firmware-and-memory` herunterladen und darin `TramliBern-3.2-TEST.zip` entpacken.
Es enthält `TramliBern/` mit vollständigem Sketch, Header und kompilierten Binärdateien,
den Speicherbericht, die Board-Einstellungen und SHA256-Prüfsummen.
GitHub verlangt für Artefaktdownloads gegebenenfalls eine Anmeldung; Aufbewahrung 30 Tage.

## Vor dem Hochladen

1. Eigene Haltestellen-/Board-Einstellungen notieren oder sichern. Die bisherige
   funktionierende 3.1-Firmware aufbewahren; Quellsicherung: `backups/TramliBern_3_1`.
2. Nur den bisherigen **klassischen ESP32, 4 MB Flash, ohne PSRAM** verwenden.
   Nicht auf ESP32-S3/C3 oder andere Hardware übertragen.
3. Arduino CLI 1.5.1 und ESP32-Core 3.3.2 installieren. Für erneutes Kompilieren
   zusätzlich ArduinoJson 7.4.2, GFX Library for Arduino 1.6.9 und XPT2046_Touchscreen 1.4.
4. Default-Partitionierung beibehalten. Kein vollständiges Löschen des Flash/NVS.

## Kompilierte Firmware manuell hochladen

Im entpackten Paket mit `arduino-cli board list` den USB-Port feststellen.
`DEIN_USB_PORT` ersetzen und erst dann selbst ausführen:

```sh
arduino-cli upload --port DEIN_USB_PORT \
  --fqbn 'esp32:esp32:esp32:CPUFreq=240,FlashSize=4M,PartitionScheme=default,PSRAM=disabled' \
  --input-dir TramliBern TramliBern
```

Alternativ `TramliBern/TramliBern.ino` in Arduino IDE öffnen, dieselben Versionen
und Board-Einstellungen wählen, kompilieren und hochladen. Falls der ESP32 nicht
in den Bootloader wechselt, die bisher bewährte BOOT-Tasten-Prozedur verwenden.
Es wurde kein Upload aus der Cloud auf dein Gerät ausgeführt.

## Danach prüfen

Blauen Stadtwechsel, roten ABFAHRTEN-Button, Wischen durch alle drei Seiten,
Umlaute der Touch-Tastatur, Haltestellenverwaltung, Live-Abfahrten und lokale Web-App
mit mehreren Boards ausprobieren. Bei Problemen 3.1 mit unveränderter Partitionierung
wieder installieren. Dynamischer Heap und tatsächliches Display-/Touch-Verhalten
können erst auf dem Gerät geprüft werden.

## Grenzen des Warnbetriebs

Nur ausdrücklich genannte Halte mit nachgewiesenen Kennungsbeziehungen sind Warnungen.
Linien-/Fahrtkandidaten und unklare Abschnitte erscheinen nicht. Eine Meldung auf einer
Haltestellenseite bedeutet keine Aussage über jede dort angezeigte Abfahrt.

Der Testfeed wird bei erfolgreichen Cloud-Testläufen auf dem Testzweig erneuert.
Er verfällt spätestens 15 Minuten nach dem Original-Quellzeitstempel. Der Workflow
veröffentlicht keine veralteten Daten. Nach Ablauf sind Warnungen ausgeblendet;
Abfahrten und übrige Funktionen laufen weiter. Ein fehlender Warntext beweist keine
Störungsfreiheit. Die Web-App zeigt den Feed-Status an.

Es gibt auf diesem Testzweig noch keinen dauerhaften periodischen Cloud-Betrieb.
Ein vorhandener Workflow-Lauf kann auf GitHub über **Re-run all jobs** erneut gestartet
werden; der Mac muss dafür nicht laufen. `main` und die bestehenden produktiven
Verkehrsmeldungs-Workflows bleiben unangetastet. Vor einem Dauerbetrieb sind eine
regelmäßige Feed-Erneuerung und die produktive Protobuf-Schnittstelle erforderlich.
