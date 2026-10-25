# TramliBern 3.2 TEST – überprüfter Entwicklungsstand

Arbeitszweig: `test/tramlibern-3.2-verified`; Entwurfs-PR:
https://github.com/JerryD007/TramliBern-Verkehrsmeldungen/pull/2

## Stand vom 23.10.2026

Die neue 3.2-TEST-Firmware ist vollständig kompiliert. 152 automatische Tests
bestehen, einschließlich eines echten positiven Schweizer Störungsfalls und
negativer Kontrollen für alle fünf gewünschten Berner Haltestellen.
**Noch keine Freigabe für einen dauerhaften Warnbetrieb:** Lauf 39890999298
bestand am 23.10.2026 alle 130 damaligen Tests, Kompilierung und den Live-Abgleich.
Die Quelle war aktuell: 521 gültige aktive Entitäten, 40.635 Selektoren, eine
bestätigte Kontrollzuordnung und keine bestätigte Warnung für die fünf Zielhalte.
Ein leerer, kurzlebiger Testfeed wurde korrekt auf dem Testzweig veröffentlicht.
Die Testwarnversorgung benötigt erfolgreiche, regelmäßig erneuerte Cloud-Läufe.
Der eigenständige Refresh-Workflow und eine getrennte Zeitplan-Datei sind vorbereitet;
Aktivierung und verbleibende Grenzen stehen in [CLOUD-3.2.md](CLOUD-3.2.md).
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
Produktivbetrieb sind kontinuierliche sichere Feed-Erneuerung,
weitere Städte/Halte sowie reale positive Trip-/Abschnittsnachweise noch offen.
Ein grüner Workflow allein ist ausdrücklich kein fachlicher Zuordnungsnachweis.


## Fortsetzung: Binäranbindung am 23.10.2026

Der Live-Abgleich liest jetzt die offizielle Protobuf-Antwort. Rohbytes werden
als `real-gtfs-source.pb` zusätzlich zum lesbaren Diagnoseabbild archiviert.
Die gemeinsame Kennungs-/Gültigkeitsprüfung bleibt unverändert. Neue Tests prüfen
unbekannte Felder in Feed, Header, Alert, Zeitraum und verschachtelter Fahrt:
Kein unbekannter Selektor darf durch das Entfernen einer Einschränkung zu einer
Warnung werden. Unbekannte übergeordnete Bedingungen sperren die betroffene Meldung
beziehungsweise den ganzen Feed.

Ein echter Binärausschnitt bestätigt den Liebegg-Selektor. Die historische
Binärquelle enthält außerdem Trip-Zusatzfelder mit Feldnummer 7, die von der
Standardbibliothek nicht erkannt werden. Diese werden bewusst nicht als
`originalTripId` geraten: die aktuelle Betreiberbeschreibung für GTFS-RT nennt
hierfür Feldnummer 8. Solche Fahrtselektoren bleiben bis zur eindeutigen
Schema-Klärung unverifizierbar. Die bereits getestete JSON-Instanzprüfung bleibt
für historische Diagnosen erhalten; der Binärpfad verwirft keine Zusatzbedingungen.

Die am 24.08.2026 angekündigten neuen `impact_period`/`communication_period`
und `originalStopId` werden noch nicht interpretiert. Unbekannte Binärfelder
werden sicher gesperrt statt als uneingeschränkt gültige Meldung behandelt.
Quelle: https://opentransportdata.swiss/de/vorankuendigung-gtfs-sa-aufteilung-der-feeds-und-weitere-neuerungen/

Stetige Feed-Erneuerung, weitere Haltestellen und reale positive Fahrt-/Abschnitts-
nachweise bleiben offen. Keine Freigabe für Dauerbetrieb durch diese Umstellung.


## Aktualitätsnachweis und Abrufreihenfolge (23.10.2026)

Der große statische Fahrplan wird nun vor dem Echtzeitfeed geladen. Dadurch
verbraucht dessen Download keine Restlaufzeit des Warnfeeds. Veröffentlichung
prüft weiterhin den unveränderten Originalzeitstempel nach der Zuordnungsprüfung.
`build_feed` lehnt auch einen fehlenden Quellzeitstempel ab; ein implizites Ersetzen
mit der aktuellen Uhrzeit ist ausgeschlossen.

`source-observation.json` und die unabhängige Fehlerprobe protokollieren SHA256,
Abrufzeit, Alter beim Abruf und eine beschränkte Auswahl von HTTP-Cache-Headern.
Authorization und Cookies werden nicht archiviert. Ein frisches HTTP-Date oder
Age=0 gilt ausdrücklich nicht als Nachweis aktueller Inhalte. Diese Diagnose
unterscheidet eine bereits alte Quelle von Alterung während der Verarbeitung.

Das offizielle Schweizer Profil 0.17, Abschnitt 11.2.4.2, bezeichnet den Timestamp
als Erstellungszeit des Feed-Inhalts. Daraus folgt keine belegte Ausnahme, um
mehrere Stunden alte Daten als aktuell zu veröffentlichen. Die 15-Minuten-Grenze
bleibt eine bewusst konservative Projektregel, keine zugesicherte Betreiber-SLA.
Quelle: https://opentransportdata.swiss/wp-content/uploads/2026/07/GTFS_Profil_Switzerland_Version_0_17_en.pdf

Die automatischen Tests prüfen zusätzlich Abrufreihenfolge, erhaltene Originalbytes,
unveränderte Quellzeitstempel, fehlende/falsche Zeitstempel und sichere HTTP-Protokolle.
Periodische Actions laufen laut GitHub nur auf dem Default-Zweig. Ein reines
`schedule:` im Testzweig würde den Dauerbetrieb daher nicht herstellen. Noch kein
externer Scheduler eingerichtet; bestehende Workflows und main bleiben unverändert.
Quelle: https://docs.github.com/en/actions/reference/workflows-and-actions/events-that-trigger-workflows

## Umstellung auf Linienmeldungen (23.10.2026)
Auf ausdrücklichen Nutzerauftrag erhält 3.2 TEST einen getrennten Feed
`data/alerts-lines.json` (Schema 3). Jede Anzeige beginnt mit `Linienmeldung <Linie>:`.
Dies bestätigt ausschließlich die Zuordnung der offiziellen Störung zu einer Linie,
die einen ausgewählten Halt im statischen Fahrplan bedient. Es bestätigt weder
Betroffenheit dieses Halts noch Richtung oder Abschnitt dort.
Originale Selektoreinschränkungen werden weiterhin geprüft. Unbekannte IDs,
Widersprüche und nicht unterstützte Fahrtzusatzfelder bleiben ausgeschlossen.
Reine Haltestellenmeldungen ohne ermittelbare Linie werden nicht übernommen.
Der bisherige Schema-2-Feed bleibt semantisch unverändert, sodass ältere Testfirmware
keine Linienmeldung als bestätigte Haltestellenwarnung interpretiert.
Die Live-Abdeckung bleibt zunächst auf die fünf konfigurierten Berner Zielhalte
beschränkt. Andere Halte und Städte erhalten dadurch noch keine Linienmeldungen.
Stop-bezogene Selektoren außerhalb der geladenen Fahrplanfamilien bleiben ohne
Verbindungsnachweis ausgeschlossen. Keine vollständige Störungsabdeckung zugesagt.
Firmware 3.1 bleibt auf dem Gerät; kein Flash und keine Änderung an main.


## SIRI-Dienst: Fortsetzung 24.10.2026

Getrennte periodische Abrufe (vollständig/ungeplant), atomarer Cache,
Versionierung/Rücknahmen und Neustart-/Ablaufsperren sind im Dienst implementiert.
52 SIRI-Tests bestehen lokal einschließlich historischem HTTP-Replay.
Der Container und seine sichere 503-Antwort ohne Quelldaten werden im neuen
SIRI-Testworkflow unabhängig von der API-Verfügbarkeit geprüft.

**Nicht fertig und nicht produktiv freigegeben:** Der vorhandene API-Zugang
lieferte am 24.10.2026 um 08:08 Uhr Europe/Zurich bei beiden offiziellen
SIRI-Endpunkten HTTP 403. Der genaue Ablehnungsgrund ist nicht bestätigt.
Nachweis: https://github.com/JerryD007/TramliBern-Verkehrsmeldungen/actions/runs/40068999229
Es fehlen weiterhin die belegte SIRI-Linien-/Haltestellenbrücke, öffentliches
Hosting und die daran anschließende Firmwareprüfung. Keine Umstellung der
installierten Firmware und keine Änderungen an main in dieser Fortsetzung.
Details und Startkonfiguration: [service/siri/README.md](service/siri/README.md).
