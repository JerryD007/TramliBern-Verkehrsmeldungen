# SIRI-SX-Abfragedienst – Forschungsprototyp, 23.10.2026

Dieser separate Dienst verarbeitet das normative `PublishingAction`-Profil wie
im offiziellen SIRI-Monitor dokumentiert. Er ist **noch kein Ersatz des
Firmwarefeeds** und wurde **nicht öffentlich bereitgestellt**. Main, Firmware,
3.1-Sicherung und bestehende Workflows werden durch diese Dateien nicht geändert.

## Bereits ausgeführt

Der Produktions-XML-Endpunkt aus der Monitor-Konfiguration war am 23.10.2026
ohne Authentifizierung erreichbar (HTTP 200). 90.247.725 Byte, 1.183 Situationen,
Quellzeit 16:59:52 UTC. SHA256 und historisches Prüfergebnis stehen in
`ci/fixtures/siri/evidence-20261023.json`.
Der Parser arbeitet inkrementell, hält nicht das gesamte XML im Speicher und
extrahierte 998 unterstützte Linien-Datensätze. 14 sehr große Situationen wurden
wegen Ressourcenlimits ausdrücklich ausgelassen. Das ist keine vollständige
Abdeckung aller Schweizer Störungen.

Realer positiver Nachweis: BERNMOBIL, `ch:1:sboid:100626`, Linie `85:827:12`,
Liebegg `ch:1:sloid:90035`, Richtung `H`, gültig 19.–26.10.2026. Die kleine
XML-Fixture enthält die echte, unverändert inhaltlich serialisierte Situation;
der umgebende Siri-Envelope wurde für den historischen Test konstruiert.
Die Tests werten sie am historischen Quellzeitpunkt aus, nicht als heutige Warnung.

## Schnittstelle

`GET /v1/line-messages?operator_ref=ch:1:sboid:100626&line_ref=85:827:12`

Antwort: `schema=tramlibern.siri.lines.v1`, Original-Quellzeit, Ablauf,
`Linienmeldung 12: …`, originale Betreiber-/Linien-/Situationskennungen,
angegebene Richtung und angegebene Halte. Die Antwort sagt ausdrücklich
`selected_stop_relevance=not_verified`. Keine Zuordnung per bloßer Liniennummer.
Dieses Schema ist absichtlich nicht Firmware-Schema 3; SIRI-LineRef wird niemals
als GTFS-route_id ausgegeben. Die Firmware muss es derzeit ablehnen.

HTTP 503 bei fehlendem, altem oder ungültigem Feed; ein Fehler wird niemals als
bestätigte störungsfreie Lage ausgegeben. HTTP 200 mit leeren Meldungen bedeutet
nur: keine passende Meldung im unterstützten Teil dieses Snapshots.
Alle Antworten maximal 22.000 Byte/32 Meldungen, `Cache-Control: no-store`.
Quellalter maximal 15 Minuten, Zukunftstoleranz 120 Sekunden. Abruf und
HTTP-Abfrage erneuern den Originalzeitstempel nicht. Endzeit ist exklusiv.

Unterstützt: veröffentlichte Situationen, explizite Linien im PublishingScope
`line` oder `stopPlace`, deutscher Kurztext, Gültigkeit sowie Veröffentlichungs-
fenster auf Situation und Aktion. Fahrtbezogene PublishingScopes werden nicht
zu Linienwarnungen erweitert. Nicht alle Erweiterungen des Schweizer Profils
sind implementiert; deshalb keine Produktionsfreigabe. Mehrdeutige doppelte
Situationen werden unterdrückt, defekte Zeiträume schließen die Situation aus.
DTD/Entities, unvollständige Dokumente und Ressourcenüberschreitungen werden
abgefangen. Fehlerhafte Downloads ersetzen keinen vollständigen Cache.

## Start und Betrieb

Historischer Offline-Test:

```
python -m unittest discover -s ci -p 'test_siri_service.py' -v
SIRI_SNAPSHOT_PATH=/absolute/path/to/source.xml python -m service.siri.app
```

Container aus dem Repository-Root:

```
docker build -f service/siri/Dockerfile -t tramlibern-siri .
docker run --rm -p 8080:8080 -e SIRI_SOURCE_URL=https://tools.opentransportdata.swiss/data/siri-sx/siri_sx-latest-prod.xml -e SIRI_INTERVAL=3600 tramlibern-siri
```

Das ist **Forschungsbetrieb**: der ungesicherte Aktualisierungsvertrag des
Monitor-Spiegels rechtfertigt kein minutenschnelles Polling. Der Prototyp erlaubt
dort höchstens stündliches Laden und verweigert danach veraltete Antworten.
Dieser Modus kann folglich keine durchgehende Warnversorgung liefern.
Der HTTP-Request des ESP32 löst keinen 90-MB-Download aus; ein einziger Hintergrund-
thread lädt, validiert und ersetzt den Cache atomar. Nur ein Prozess/Replica
betreiben; mehrere Instanzen würden Abruflimits vervielfachen. Vor öffentlichen
Betrieb HTTPS-Terminierung und Ressourcen-/Anfragelimits des Hosts einrichten.
Keine Zugangsschlüssel in URL, Firmware, Git oder Antwort; optionaler API-Token
nur als Laufzeitsecret. Redirects sind gesperrt, erlaubte Quell-URLs fest begrenzt.

## Was für den produktiven direkten Weg noch fehlt

1. **Offizieller getrennter Abruf:** Der Cookbook empfiehlt vollständige Daten
   ein-/wenige Male täglich und `/la/siri-sx-unplanned` laufend. Der Adapter
   erlaubt diese URLs mit vorhandenem Laufzeitsecret, aber ihre Berechtigung
   wurde hier nicht nachgewiesen. Ein Modus mit nur dem ungeplanten Feed deckt
   geplante Baustellen nicht ab. Die versionierte Zusammenführung beider Feeds
   einschließlich Rücknahmen muss vor Freigabe implementiert und real getestet
   werden. Keinen neuen Token vom Nutzer verlangen, bevor bestehender
   serverseitiger Zugang geprüft wurde.
2. **Exakte Linien-Halt-Brücke:** Die fünf GTFS-Halte sind bekannt, SIRI-LineRef
   und SBOID brauchen jedoch eine separat belegte Verbindung zu den
   Fahrplanlinien. Nummer 12 allein reicht nicht. Der Liebegg-Fall belegt die
   offizielle Linienmeldung, nicht automatisch die Relevanz für Zentrum Paul Klee.
3. **Hosting:** Kein dauerhaft laufender Container-Host wurde in dieser Sitzung
   bereitgestellt. Das verfügbare Sites-Hosting führt JavaScript/Cloudflare
   Workers aus, nicht diesen Python-Container. Ein öffentlicher Service benötigt
   passenden Host oder einen separat getesteten Streaming-Worker-Port. GitHub
   Actions ist hier CI, kein dauerhafter Webserver.
4. **Firmware:** Erst nach HTTPS-Endpunkt, belegter Linien-Halt-Brücke und
   Dauerprüfung einen kompakten Adapter integrieren und den Sketch kompilieren.
   Keine neue Firmware allein aufgrund erfolgreicher Parser-Tests hochladen.

## Referenzen

- https://opentransportdata.swiss/de/cookbook/event-cookbook/siri-sx/
- https://github.com/openTdataCH/siri-sx-situation-monitor
- Dort: `apps/siri-sx-demo-client/ts/config/app_config.ts` und
  `models/pt_situation_element.ts` (main, geprüft 23.10.2026).

Der offizielle Monitor dient als Strukturreferenz; sein TypeScript-Code wurde
nicht kopiert. Eigene Python-Implementierung. Primäre Auswahl aus
`PublishAtScope/Affects`, nicht aus den optionalen, nicht empfohlenen
`PtSituation/Affects` oder `Consequences/Affects`.
