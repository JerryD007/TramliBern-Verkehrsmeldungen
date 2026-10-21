# TramliBern 3.1 TEST – Linienzuordnung

Dies ist ein **isolierter Entwicklungszweig** von der funktionierenden 3.0-Testfirmware. Die 3.0-Firmware auf dem Gerät, `main`, beide Produktions-Workflows und der offene PR Nr. 1 bleiben unverändert.

## Status und Sicherheitsregel

- `ci/gtfs_route_verifier.py` vergleicht **exakte** GTFS-Verknüpfungen `routes.route_id → trips.route_id → stop_times.trip_id → stops.stop_id`.
- Nur echte IDs aus den referenzierten GTFS-Tabellen werden verwendet; eine Liniennummer wie `7` wird nicht aus einem fremden `route_id`-Text abgeleitet.
- Eine Route, die an einer Haltestelle hält, begründet zunächst **nur einen Kandidaten**. Eine Störung kann einen anderen Abschnitt, die Gegenrichtung oder einen anderen Betriebszeitraum betreffen. Die 3.1-Testfirmware zeigt deshalb **keine automatische Linienwarnung** aufgrund dieser Beziehung allein.
- Die Web-App unter `/alerts-status` zeigt die zuletzt beobachteten Linien pro Haltestelle, ausdrücklich als **Abfahrtsdaten, kein GTFS-Beweis**. Die bisher verifizierten direkten Haltestellenmeldungen funktionieren weiterhin.
- Das bestehende GitHub-Pages-Format `alerts.json` enthält nur `generated`, `alerts` mit `stops`, `text`, `end`; dort fehlen derzeit Route-IDs und ein verifizierter Segment- und Richtungsbezug. Die bestehende Veröffentlichungslogik wird absichtlich nicht verändert.

## Vor einer Freischaltung von Linienwarnungen erforderlich

1. Eindeutige GTFS-Quelle mit dokumentierter Version und eindeutigen `stop_id` / SLOID-Zuordnungen nachweisen.
2. Originale GTFS-Service-Alert-Selektoren einschließlich `routeId`, `tripId`, `stopId`, Richtungs-/Zeitbezug analysieren und nachvollziehbar gegen statische GTFS-Daten desselben Datenstandes verknüpfen.
3. Mit tatsächlichen Meldungen gegenprüfen und nur hinreichend bestätigte Auswirkungen auf die gewählte Haltestelle / Linie im Display veröffentlichen.
4. Vor dem Flashen eine lokale Sicherung der 3.0-Firmware erstellen.

## Tests

Der getrennte GitHub-Workflow führt weiterhin ESP32 Arduino CLI Compilation, Speicherbericht sowie alle `ci/test_*.py` Regressionstests aus. Keine GitHub-Secrets für die Offline-Tests nötig.
