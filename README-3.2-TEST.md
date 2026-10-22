# TramliBern 3.2 TEST – sichere GTFS-Service-Alert-Zuordnung

Der neue Zweig wurde von `test/tramlibern-3.1-route-verification` erstellt.
Die funktionierende Firmware bleibt exakt wie in 3.1; der ESP32 wird nicht aktualisiert.
`main`, GitHub Pages, die veröffentlichten Meldungen und vorhandene Verkehrs-Workflows werden nicht geändert.

## Neue Prüfungen

`ci/gtfs_alert_segments.py` liest statische GTFS-Tabellen (`routes.txt`,
`trips.txt`, `stop_times.txt`, `stops.txt`) und untersucht einzelne
GTFS-Service-Alert-Selektoren mit `routeId`, `tripId`, `stopId`.

* **confirmed_stop**: Eine ausdrücklich genannte `stopId` passt exakt zur gewählten
  Haltestelle (oder deren Plattform), ist in `stops.txt` vorhanden, und optionale
  `routeId`/`tripId`-Bedingungen stimmen mit den GTFS-Verknüpfungen überein.
* **candidate**: Eine Route oder Fahrt bedient die Haltestelle, aber im Alert
  fehlt `stopId`. Es gibt **keinen** Beweis, dass der gestörte Streckenabschnitt
  diese Haltestelle betrifft. Kandidaten dürfen nicht auf der Abfahrtstafel
  als bestätigte Warnungen erscheinen.
* **no_match**: Unbekannte Kennungen, widersprüchliche Route/Fahrt, andere
  Haltestelle oder nicht vorhandene Verbindung.

Der Prüfcode verwendet **niemals** die Textzahl einer Tram-/Buslinie als Ersatz
für `route_id`. Rückwärts-/Gegenrichtungs- und Segmentlogik können nur mit
echten Selektoren und passenden statischen Daten beurteilt werden. Ein
`stopId`-Selector markiert eine betroffene Haltestelle, aber sagt nicht
automatisch etwas über jede dort verkehrende Linie aus.

## Nächste reale Datenerhebung

Für belastbare Streckenwarnungen muss ein exakter Datensatz der Originalmeldungen
(inklusive aller `informedEntity`-Selektoren, Gültigkeitszeiträume sowie
`routeId`/`tripId`/`stopId`) mit einer **kompatiblen** GTFS-Version verglichen
werden. Zusätzlich sind bei Streckenstörungen Richtung und betroffenes Segment
zu ermitteln – soweit die Quelle dies überhaupt bereitstellt.

Die bisherigen öffentlichen `alerts.json`-Daten enthalten nur
`generated`, `alerts[].stops`, `text`, `end`. Sie reichen für
Streckenwarnungen nicht aus. Deshalb keine Änderung des produktiven Feeds.

## Testen

`python3 -m unittest discover -s ci -p 'test_*.py' -v`

GitHub Actions auf diesem Testzweig prüft alle Python-Tests und kompiliert
zusätzlich den unveränderten ESP32-Sketch. Kein Mac und kein Token nötig.
