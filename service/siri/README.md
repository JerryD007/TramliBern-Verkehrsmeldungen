# TramliBern SIRI-SX-Dienst – Stand 24.10.2026

**Nicht produktiv freigegeben.** Der Dienst ist implementiert und kontrolliert
getestet, aber nicht öffentlich gehostet und nicht an die Firmware angeschlossen.
Die neue Live-Prüfung mit dem vorhandenen GitHub-Secret liefert auf **beiden**
offiziellen SIRI-Endpunkten HTTP 403. Das belegt eine Zugriffsablehnung, nicht deren
genaue Ursache. Keine weiteren Tokens aus Screenshots oder öffentlichen Beispielen
werden benutzt. Zugriff prüfen: https://api-manager.opentransportdata.swiss/

Nachweis am 24.10.2026, 08:08 Uhr Europe/Zurich:
https://github.com/JerryD007/TramliBern-Verkehrsmeldungen/actions/runs/40068999229
Artefakt: `siri-access-evidence`, ID 12550835243. Der Test gibt ausschließlich
HTTP-Status, Zeitpunkte und sichere Zähler aus, niemals den Token oder Fehlerbody.

## Implementierter Datenweg

`runtime.py` betreibt genau zwei Hintergrundabrufe in einem Prozess:

- `/la/siri-sx`: alle 12 Stunden; nach Fehler frühestens nach 30 Minuten.
- `/la/siri-sx-unplanned`: alle 60 Sekunden nach Abschluss des vorherigen Abrufs.
- HTTP-Anfragen bedienen ausschließlich den atomar ausgetauschten Speicher-Cache;
  sie starten keinen großen Download und verändern keinen Quellzeitstempel.
- Vollständige ungeplante Snapshots ersetzen den vorherigen Snapshot. Verschwundene
  ungeplante Ereignisse werden nicht aus dem alten Vollfeed wiederhergestellt.
- Neuere Versionen und Rücknahmen haben Vorrang; gleichversionige Widersprüche,
  doppelte IDs und ungültige Daten sperren die betreffende Situation.
- Rücknahmen geplanter Ereignisse bleiben bis zur passenden neueren Vollversion
  erhalten. Ein neuer Download mit alter Ereignisversion darf sie nicht aufheben.
- Beide Quellen sind erforderlich: ungeplant maximal 15 Minuten alt, Vollsnapshot
  maximal 24 Stunden alt. Diese getrennten Grenzen folgen dem empfohlenen seltenen
  Vollabruf; sie sind Projektregeln und keine Betreiber-SLA. Bei Erwerb eines neuen
  Snapshots gilt zunächst ebenfalls die 15-Minuten-Prüfung. Bereits alte Quelldaten
  werden nicht durch den Abruf frisch.
- Fehlende/veraltete Quelle: HTTP 503; keine Behauptung einer störungsfreien Lage.
  Nach Prozessneustart bleibt der Dienst gesperrt, bis beide Quellen geladen sind.

Der Parser wertet ausschließlich den unterstützten normativen `PublishingAction`-
Teil aus. Unterstützt sind veröffentlichte Linien-/StopPlace-Aktionen mit explizitem
Betreiber, LineRef, deutschem Kurztext, Gültigkeit und Veröffentlichungsfenstern.
Fahrtbezogene Scopes werden nicht zu Linienmeldungen erweitert. DTD/Entities,
zu tiefe XML-Bäume, unvollständige Dokumente und Ressourcenüberschreitungen werden
abgefangen. Sehr große einzelne Situationen werden mit Zähler ausgelassen; daher
wird keine vollständige Schweizer Störungsabdeckung zugesagt.

## ESP32-kompakte HTTP-Schnittstelle

`GET /v1/line-messages?operator_ref=ch:1:sboid:100626&line_ref=85:827:12`

Schema `tramlibern.siri.lines.v1`, originale Quellzeitpunkte, Ablauf, Kennungen und
Text `Linienmeldung 12: …`. Maximal 32 Meldungen/22.000 Byte, `Cache-Control: no-store`.
Richtung und angegebene Halte bleiben als Quelldaten erhalten. Die Ausgabe sagt
weiterhin `selected_stop_relevance=not_verified`: Es gibt noch keine freigegebene
SIRI-LineRef-/GTFS-/Haltestellenbrücke. Keine Zuordnung anhand einer Liniennummer,
kein Umdeuten der SIRI-LineRef als GTFS-route_id. Das Schema ist absichtlich nicht
Firmware-Schema 3; bestehende Firmware darf diesen Prototyp nicht übernehmen.

## Tatsächliche Nachweise

Am 23.10.2026 war der Produktions-XML-Spiegel aus der offiziellen Monitor-Konfiguration
ohne Authentifizierung erreichbar. 90.247.725 Byte, 1.183 Situationen, Quellzeit
16:59:52 UTC, 998 unterstützte Linien-Datensätze, 14 wegen Größenlimits ausgelassene
Situationen. SHA256 und Ergebnis: `ci/fixtures/siri/evidence-20261023.json`.
Dieser Spiegel ist eine Forschungsquelle ohne hier belegten Polling-/Betriebsvertrag,
kein Ersatz für die jetzt abgelehnte offizielle API-Berechtigung.

Realer positiver Fall: BERNMOBIL `ch:1:sboid:100626`, Linie `85:827:12`, Liebegg
`ch:1:sloid:90035`, Richtung H, gültig 19.–26.10.2026. Die XML-Fixture enthält die
inhaltlich unveränderte, neu serialisierte Situation in einem konstruierten
Replay-Envelope. Alle Replay-Tests verwenden ausdrücklich die damalige Zeit.
Dies beweist die Linienmeldung, noch nicht ihre Relevanz für Zentrum Paul Klee.

52 SIRI-Tests prüfen Parser, Versionen, Rücknahmen, Ablauf, Abrufintervalle, Fehler-
Wartezeiten und einen echten lokalen HTTP-Aufruf mit historischem Replay.
Die automatischen Tests laufen im bestehenden Testzweig und zusätzlich im neuen
SIRI-Testworkflow. Live-Zugriff ist ein eigener, derzeit scheiternder Nachweis.

## Start / Container

```
python -m unittest discover -s ci -p 'test_siri*.py' -v
docker compose -f service/siri/compose.yaml up --build -d
```

`OPENTRANSPORT_API_TOKEN` muss als Laufzeitsecret verfügbar sein, niemals in Git,
Firmware oder URLs. `compose.yaml` bindet nur `127.0.0.1:8080`, setzt Speicher-/CPU-
Grenzen, startet bei Absturz neu und verwendet einen unprivilegierten Benutzer.
Öffentlicher Betrieb benötigt zusätzlich TLS-Terminierung und einen Host. Genau
**ein** Worker/Replica; mehrere Instanzen würden die Datenabrufe vervielfachen.
Docker-Start ist noch kein Nachweis gültiger API-Rechte oder dauerhafter Datenversorgung.

## Noch offen, kein Firmware-Update

1. Funktionierender Zugriff auf beide offiziellen APIs. Der vorhandene Schlüssel
   wurde benutzt; ein neuer Schlüssel wird nicht ohne Ursache verlangt.
2. Belegte SIRI-Linie–GTFS-Linie–Haltestelle-Beziehungen für die fünf Zielhalte.
3. Passender dauerhafter Host. Das verfügbare Sites-Hosting kann JavaScript-Workers,
   aber nicht diesen Python-Container starten. Eine Worker-Portierung wäre eine
   zusätzliche Implementierung mit eigener Speicher-/Streaming- und Zugriffskontrolle.
4. Reale Dauerprüfung und anschließend Firmware-Adapter, vollständiger Build und
   kompilierte Testfirmware. Kein Zugriff oder Flash am physischen ESP32.

Main, bestehende Verkehrsworkflows und 3.1-Sicherung bleiben unverändert.

## Referenzen

- https://opentransportdata.swiss/de/cookbook/event-cookbook/siri-sx/
- https://api-manager.opentransportdata.swiss/portal/catalogue-products/tedp_siri_sx_unplanned-1
- https://github.com/openTdataCH/siri-sx-situation-monitor
  (`app_config.ts` und `pt_situation_element.ts`, geprüft 23.10.2026).

Eigene Python-Implementierung; kein Monitor-TypeScript kopiert. Die Quelle empfiehlt
PublishingAction statt optionalem PtSituation/Affects oder Consequences/Affects.
