# TramliBern 3.2 – geprüfte Zuordnung, Entwicklungsstand

Arbeitszweig: `test/tramlibern-3.2-verified`, abgeleitet von
`test/tramlibern-3.2-alert-segments` bei Commit
`5e55c82b16e2e2b8b85c42b5b8a7b66017019642`.

## Nachgewiesene Fehler der vorherigen Diagnose

- `directionId` wurde gelesen, aber nicht ausgewertet.
- Plattformen wurden per Kennungspräfix statt `stops.parent_station` zugeordnet.
- Die Live-Auswertung und die Unit-Tests verwendeten unterschiedliche Zuordnungslogik.
- Unbekannte IDs und echte Nichtbetroffenheit waren teilweise nicht unterscheidbar.
- Der Zähler `not_matched` wurde nicht erhöht.
- Verschachtelte Trip-Selektoren konnten widersprüchliche Angaben überschreiben.
- Veraltete Feed-Header und DIFFERENTIAL-Feeds wurden nicht abgefangen.

Die neue Diagnose verwendet denselben Verifizierer wie die Tests. Jede mitgelieferte
unterstützte Bedingung muss stimmen. Unbekannte Bedingungen werden nicht verworfen.

## Bestehende reale Evidenz (vor dieser Weiterentwicklung)

GitHub-Lauf: https://github.com/JerryD007/TramliBern-Verkehrsmeldungen/actions/runs/39811978681

Am 22.10.2026 meldete der Lauf 523 aktive Entitäten, 40.201 Selektoren,
690 unterschiedliche routeId, davon 380 exakt im statischen Fahrplan gefunden;
1.854 Selektoren mit stopId und 17 Kennungen für die ausgewählten Haltestellen.

690 − 380 = 310 nicht gefundene Linienkennungen. Das beweist eine unvollständige
Kennungsübereinstimmung, aber **nicht deren Ursache**. Ein Versionswechsel, andere
Kennungsformate oder tatsächlich fehlende Berner Störungen sind damit noch nicht
abschließend unterschieden. Es ist insbesondere kein Beweis für Störungsfreiheit.

## Sicherheitsgrenze

- `confirmed_stop`: ausdrücklich genannter Halt und alle mitgelieferten
  unterstützten Bedingungen stimmen mit den statischen Daten überein.
- `candidate`: Route/Fahrt bedient den Halt, betroffener Abschnitt fehlt.
- `no_match`: die bekannten Bedingungen passen nicht zur gewählten Haltestelle.
- `unverifiable`: unbekannte IDs, fehlende notwendige Daten, widersprüchliche oder
  noch nicht unterstützte Bedingungen. Keine Displaywarnung.

Richtungen werden exakt als 0/1 geprüft; es gibt keine Vermutung anhand von
Liniennummern, Namen oder Text. Stationen und Plattformen werden nur über die
explizite Elternbeziehung verbunden. Benachbarte Plattformen sind nicht austauschbar.
Zwei Selektoren werden niemals zu einer künstlichen Streckenangabe kombiniert.
Zwischen zwei genannten Halten werden keine weiteren betroffenen Halte erfunden.

Trip-Selektoren mit startDate/startTime/scheduleRelationship werden derzeit
konservativ als nicht zuordenbar behandelt. Eine vollständige Instanzauflösung
mit Betriebstagen, Frequenzen und Nachtfahrten ist noch nicht implementiert.
Die Prüfung bestätigt räumliche Selektorbeziehungen, nicht Verspätungen einzelner
Live-Abfahrten. Eine erfolgreiche Kompilierung bestätigt keine Display-, Touch-
oder WLAN-Funktion am Gerät.

## Tests und Daten

`python3 -m unittest discover -s ci -p 'test_*.py' -v`

Kontrollierte Tests prüfen IDs, parent_station, Richtung, widersprüchliche und
unbekannte Selektoren, Gültigkeitslücken, exklusives Periodenende, alte und zukünftige
Feed-Header, gelöschte Entitäten, doppelte IDs und defekte GTFS-Tabellen.

Die neue Cloud-Diagnose speichert Original-JSON, statischen SHA256, Download-URL,
exakte ausgewählte GTFS-Zeilen, Kennungsüberschneidungen und Gründe für jede
Ergebnisklasse. Ein grüner Workflow bedeutet zunächst nur erfolgreiche Ausführung.
Die inhaltlichen Ergebnisse sind separat zu prüfen.

## Firmware und Sicherung

Die Firmware ist bis zur Prüfung der neuen realen Ergebnisse unverändert 3.1 TEST.
Eine zusätzliche Sicherung steht in `backups/TramliBern_3_1` mit SHA256-Prüfsumme.
Der neue Compile-Aufruf exportiert Binärdateien und Sketch nach `reports/firmware`;
der Speicherbericht bleibt in `reports/memory.md`.

`ci/verified_feed.py` bereitet den späteren v2-Testfeed vor, ist noch nicht an die
Firmware oder einen Veröffentlichungsworkflow angeschlossen. Kandidaten gelangen
nicht in den Feed. Er läuft spätestens nach 15 Minuten ab und überschreitet weder
32 Meldungen noch das 22.000-Byte-Limit. Der produktive Feed bleibt unverändert.

## Quellen

- https://gtfs.org/documentation/realtime/reference/ (EntitySelector, TimeRange, FeedHeader)
- https://gtfs.org/documentation/schedule/reference/ (stops.parent_station, trips.direction_id)
- https://opentransportdata.swiss/en/cookbook/event-cookbook/gtfs-sa/
- https://opentransportdata.swiss/en/cookbook/realtime-prediction-cookbook/gtfs-rt/

Die Schweizer JSON-Schnittstelle ist laut Betreiber für Testzwecke vorgesehen.
Vor einem regulären Produktivbetrieb ist die Protobuf-Anbindung erforderlich.

## Zusätzlich am 22.10.2026 direkt geprüft

Der aktuelle offizielle GTFS-Fahrplan enthält die fünf Stations-SLOIDs nicht
unmittelbar als `stop_id`. Stattdessen stehen sie als `original_stop_id` an
Stationszeilen mit `stop_id = Parentch:1:sloid:…` und `location_type = 1`.
Die 17 Plattformen verweisen per `parent_station` auf diese Zeilen.
Die Zuordnung erfolgt nun anhand dieser expliziten, eindeutigen Datenbeziehung.
`ci/fixtures/swiss_selected_stops.csv` enthält die unveränderten Originalzeilen;
Herkunft und Archivprüfsumme stehen in `ci/fixtures/PROVENANCE.json`.
Dies bestätigt die Stationsstruktur, **keine aktuelle Störung**.

Ein neuer Live-Lauf fand mindestens einen leeren/umgekehrten Gültigkeitszeitraum.
Solche Meldungen werden jetzt einzeln mit Fehlergrund ausgesondert; gültige
Meldungen werden weiter untersucht. Defekte Zeiträume werden nicht repariert oder
als unbegrenzt gültig interpretiert.

Die unveränderte 3.1-Firmware wurde auch lokal in der Cloud mit Arduino CLI 1.5.1
und den festgelegten Bibliotheken erfolgreich kompiliert:
Flash 1.253.987 / 1.310.720 Byte = 95,67 %, Reserve 56.733 Byte;
statisches RAM 58.944 / 327.680 Byte = 17,99 %.
Das ist ein Basiswert für 3.1, noch kein Kompiliernachweis einer neuen 3.2-Firmware.
