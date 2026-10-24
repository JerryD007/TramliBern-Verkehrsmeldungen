# TramliBern SLIM TEST

Neue Variante ohne Verkehrsmeldungen, GTFS-Abgleich, SLOID-Abfragen und
Störungsdiagnose. Abfahrten, Städte, Touch-Tastatur, Buttons, Wischen, Uhr und
Batterieanzeige sowie Web-App und Multi-Board-Verwaltung bleiben im Sketch.

Die fünf Plätze sind unabhängig: Seite 1 = Plätze 1/2, Seite 2 = Plätze 3/4,
Seite 3 = Platz 5 mit sieben Abfahrten. In der Web-Suche den Zielplatz auswählen.
Verschieben in freie Plätze ist erlaubt; belegte Plätze werden getauscht.
Löschen leert ausschließlich den gewählten Platz. Jede Stadt merkt sich ihre
letzte Seite auch nach Neustart. Alte Stadt-/Haltestelleneinstellungen werden
übernommen. Kein vollständiges Löschen des Flash-Speichers beim Upload wählen.

## Sicher testen

1. Funktionierenden alten Sketch als Rückfallmöglichkeit behalten.
2. ZIP entpacken, TramliBern/TramliBern.ino in Arduino öffnen.
3. Bisherige Bibliotheken und ESP32 Dev Module verwenden: 4 MB Flash,
   Default-Partitionierung, PSRAM deaktiviert; keine Partition vergrößern.
4. Erst überprüfen/kompilieren. Upload ausschließlich selbst ausführen.
5. Seite 3 zuerst belegen, Seite 1/2 leer lassen. Neustart und Stadtwechsel
   prüfen. Danach Touch, Web-App, beide Buttons, Swipe und Multi-Board testen.

Cloud-Kompilierung ersetzt keinen realen Touch-/WLAN-/Displaytest.
Keine automatische Installation, kein Merge. Bestehende Testzweige und main
bleiben unverändert. Die alten Server-Workflows werden nicht durch diese
Firmware abgeschaltet; sie sind vom neuen Sketch vollständig entkoppelt.

## Korrektur: Zielseite und Entfernen am Touchdisplay
Vor der Haltestellensuche ist ein freier Platz der aktuell angezeigten Seite
vorausgewählt. Der Zielplatz wird in die Suchergebnisse übernommen. Ist diese
Seite voll, muss ein anderer freier Platz ausdrücklich gewählt werden.
Am Board den Haltestellentitel antippen, unten HALTESTELLE ENTFERNEN wählen
und mit JA, ENTFERNEN bestätigen. ZURUECK und ABBRUCH löschen nichts.
LOESCH auf der Tastatur entfernt weiterhin nur ein Zeichen des Suchbegriffs.
Die übrigen Haltestellen und die aktuelle Seite bleiben beim Entfernen erhalten.
