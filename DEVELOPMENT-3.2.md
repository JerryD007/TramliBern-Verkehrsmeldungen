# TramliBern 3.2 TEST – überprüfter Entwicklungsstand

Arbeitszweig: `test/tramlibern-3.2-verified`; Entwurfs-PR:
https://github.com/JerryD007/TramliBern-Verkehrsmeldungen/pull/2

## Stand vom 23.10.2026

Die neue 3.2-TEST-Firmware ist vollständig kompiliert. 110 automatische Tests
bestehen, einschließlich eines echten positiven Schweizer Störungsfalls und
negativer Kontrollen für alle fünf gewünschten Berner Haltestellen.
**Noch keine Freigabe für einen dauerhaften Warnbetrieb:** Der jüngste Live-Abruf
lieferte einen veralteten Quellzeitstempel und wurde korrekt abgewiesen.
Die Testwarnversorgung benötigt erfolgreiche, regelmäßig erneuerte Cloud-Läufe.
Eine erfolgreiche Kompilierung bestätigt keine Display-, Touch- oder WLAN-Funktion
auf dem Gerät. Es fand kein Zugriff auf den physischen ESP32 statt.

## Tatsächlich nachgewiesen

Historische Originalquelle: Run 39816450561, Artefakt 12441354368.
Die Wiederholung wertet die Originaldaten ausdrücklich zum damaligen Headerzeitpunkt
1792687244 aus, nicht als aktuelle Meldungen. Evidenz und kleine unveränderte
GTFS-Zeilenauszüge: `ci/fixtures/real_20261022/`.

- 523 gültige aktive Alert-Entitäten, 40.153 Selektoren; zwei Entitäten mit
  ungültigen Zeiträumen einzeln ausgesondert.
- 22 statische Stations-/Plattformzeilen für Giacomettistrasse, Ostring,
  Zentrum Paul Klee, Burgernziel und Brunnadernstrasse korrekt gefunden.
  Stationszeilen heißen `Parentch:1:sloid:…`; die explizite `original_stop_id`
  löst die SLOID eindeutig auf. Plattformen werden über `parent_station` verbunden.
- **Positiver Kontrollfall Bern, Liebegg:** Alert
  `c3b86af6-6649-536e-a2cc-fe8249ad546a`, `agencyId=827`,
  `routeId=92-12-j26-1`, `stopId=ch:1:sloid:90035`, `directionId=0`.
  Alle Bedingungen stimmen mit echten statischen Zeilen überein.
  Derselbe Alert wird für Zentrum Paul Klee und die vier anderen Zielhalte
  ausgeschlossen. Das ist ein fachlicher Nachweis gegen pauschale Linienwarnungen.
- Für die fünf Zielhalte gibt es in dieser Aufnahme **keine bestätigte Warnung**.
  Das beweist keine Störungsfreiheit: nicht zuordenbare Selektoren bleiben bestehen.
- 671 unterschiedliche routeId: 371 exakt im Fahrplan 2026 vorhanden, 300 fehlen.
  Davon sind **289 exakt im Fahrplan 2027 nachgewiesen**, elf bleiben ungeklärt.
  Keine Ersetzung von `j27` durch `j26`, keine Namens- oder Liniennummernheuristik.
  Die 2027-Tabellenmitglieder `routes.txt`/`feed_info.txt` wurden vollständig aus
  einem teilweise heruntergeladenen offiziellen Archiv extrahiert und ihre ZIP-CRC
  geprüft; es wird kein vollständiger 2027-Archivtest behauptet.
- Fahrplan 2026: Version 20261021, gültig 20251214–20261212,
  SHA256 `3c521698af71f2fda37ae9ac748eda0107f923305615bd25383bc5976df8c3d4`.
  Fahrplan 2027: Version 20261021, gültig 20261213–20271211.

Der Live-Lauf 39861621301 wurde am 22.10.2026 um 22:00 UTC blockiert:
Die Quelle meldete 20:51:38 UTC (1792702298), also rund 69 Minuten Alter.
Eine unabhängige Abfrage der offiziellen Protobuf-Schnittstelle bestätigte um
22:14:09 UTC denselben Zeitstempel (4.951 Sekunden Alter). Es handelt sich somit
nicht nur um einen veralteten JSON-Abruf. Die 15-Minuten-Sperre wurde nicht gelockert. Veraltete Aufnahmen werden ausschließlich für klar gekennzeichnete historische
Diagnosen mit dem statischen Fahrplan verknüpft; die Veröffentlichung prüft danach
erneut den Originalzeitstempel und scheitert. Alte Daten erhalten keinen neuen
scheinbar frischen Gültigkeitszeitraum.

## Welche Störungen unterstützt werden

`confirmed_stop` bedeutet: Halt ausdrücklich genannt und jede mitgelieferte
unterstützte Bedingung verifiziert. Nur solche Ergebnisse werden als `explicit_stop`
in den gesonderten v2-Testfeed aufgenommen. Geprüft werden exakte routeId/tripId,
agencyId, routeType, stopId, directionId und explizite Stations-/Plattformbeziehungen.

Planmäßige Trip-Instanzen können nun zusätzlich anhand von `originalTripId`,
`startDate` mit `calendar.txt`/`calendar_dates.txt` und `startTime` mit der tatsächlichen
ersten Abfahrt geprüft werden. Zeiten über 24 Uhr werden nicht auf den Folgetag
umgerechnet. Frequenzfahrten und nicht planmäßige Instanzen bleiben unverifizierbar.
Reale Widersprüche zwischen Trip- und Original-Trip-ID werden nicht repariert.
Die Instanzregeln sind kontrolliert getestet; eine echte positive Warnung mit
vollständigem Trip-Instanzselektor ist noch nicht nachgewiesen.

- `candidate`: Route/Fahrt bedient Halt, betroffener Abschnitt fehlt. Keine Warnung.
- `no_match`: bekannte Bedingungen passen nicht. Keine Warnung.
- `unverifiable`: unbekannte IDs, notwendige Daten fehlen oder widersprechen sich,
  nicht unterstützte Bedingungen. Keine Warnung.

Zwei Selektoren bilden keinen künstlichen Abschnitt. Zwischen genannten Halten
werden keine betroffenen Zwischenhalte erfunden. Geschwisterplattformen sind nicht
austauschbar. Fehlende Richtung wird nicht geraten. Eine ausdrücklich auf die
Station bezogene Meldung ohne Richtungsbeschränkung darf diese Station betreffen.
Die Warnung auf einer Haltestellenseite behauptet nicht die Betroffenheit jeder
angezeigten Abfahrt.

## Firmware, Speicher und Cloud

Vollständiger Sketch: `firmware/TramliBern/TramliBern.ino` und `AlertSafety.h`.
Die Warnzuführung, Ablaufprüfung und Versionsbezeichnung wurden geändert;
bestehende Display-, Touch-, Stadt-, Abfahrts- und Web-App-Funktionen bleiben erhalten.
Die DiDok/SLOID-Zuordnung benutzt weiterhin den bisherigen IDs-Endpunkt.

| Build | Flash / 1.310.720 Byte | Flashreserve | Statisches RAM / 327.680 Byte |
|---|---:|---:|---:|
| 3.1-Sicherung | 1.253.987 (95,67 %) | 56.733 | 58.944 (17,99 %) |
| 3.2 TEST | 1.255.691 (95,80 %) | 55.029 | 58.952 (17,99 %) |

Arduino CLI 1.5.1, ESP32-Core 3.3.2, ArduinoJson 7.4.2,
GFX Library for Arduino 1.6.9, XPT2046_Touchscreen 1.4; klassischer ESP32,
240 MHz, 4 MB Flash, Default-Partitionierung, PSRAM deaktiviert.
Kein Entfernen bestehender Funktionen und keine größere Partition nötig.
Dynamischer Heap/Stack bei HTTPS, JSON, Display und Web-App bleiben am Gerät zu messen.

Der neue, getrennte Testworkflow führt Regressionen, Live-Abgleich und Kompilierung
auf GitHub aus. Nur nach bestandenem Live-Abgleich veröffentlicht er `data/alerts-v2.json`
auf diesem Testzweig. Original-Workflows und produktiver Feed bleiben unverändert.
Der Firmwarefeed unterstützt zunächst nur die fünf ausgewählten Berner Halte.
Andere Städte und frei verwaltete Haltestellen bleiben für Abfahrten verfügbar,
haben in diesem Testfeed jedoch keine verifizierte Warnabdeckung.

Feedgrenzen: maximal 32 Meldungen/22.000 Byte; Ablauf spätestens 15 Minuten nach
dem Quellzeitstempel. Die Firmware prüft Schema, IDs, Zeilenzeiträume und Feedablauf
erneut und blendet abgelaufene Daten aus. Die Web-App zeigt Fehler und Feedstatus.
Noch kein periodischer Dauerbetrieb: Testworkflow manuell erneut ausführen;
der Mac muss dafür nicht eingeschaltet sein. Geplante GitHub-Workflows laufen
nur vom Default-Zweig; dieser wird auftragsgemäß nicht verändert.

## Tests, Download, sicherer Upload

`python3 -m unittest discover -s ci -p 'test_*.py' -v`

Kontrollierte Tests: IDs, Plattformen, Richtung, Trip-Instanzen, Kalenderausnahmen,
Zeit >24 Uhr, unbekannte/widersprüchliche Selektoren, Gültigkeitslücken, exklusives
Periodenende, alte/zukünftige Header, gelöschte/doppelte Entitäten, defekte Tabellen,
Feedgröße und Wiederveröffentlichung alter Daten. Ein Host-C++-Test kompiliert die
wirklich in der Firmware verwendeten Ablauf-/ID-Prüffunktionen.
Reale Tests: Originalstationsstruktur, Liebegg-Positivkontrolle, Ausschluss aller
fünf Zielhalte, historische Ablaufprüfung, exakte 2027-Liniennachweise.

`firmware-and-memory` enthält `TramliBern-3.2-TEST.zip`, vollständige Binärdateien,
Quellen, Prüfsummen und Speicherbericht. Anleitung: [UPLOAD-3.2-TEST.md](UPLOAD-3.2-TEST.md).
3.1 bleibt in `backups/TramliBern_3_1` einschließlich Prüfsumme erhalten.
Kein automatisches Merge, kein Flash auf ein Gerät.

## Quellen und offene Grenzen

- https://gtfs.org/documentation/realtime/reference/
- https://gtfs.org/documentation/schedule/reference/
- https://opentransportdata.swiss/en/cookbook/event-cookbook/gtfs-sa/
- https://opentransportdata.swiss/en/homogenitaet-der-id-innerhalb-der-gtfs-dienste/

Die Schweizer JSON-Schnittstelle ist für Tests vorgesehen. Für regulären
Produktivbetrieb sind Protobuf-Anbindung, kontinuierliche sichere Feed-Erneuerung,
weitere Städte/Halte sowie reale positive Trip-/Abschnittsnachweise noch offen.
Ein grüner Workflow allein ist ausdrücklich kein fachlicher Zuordnungsnachweis.
