# Regelmäßige Warnversorgung – vorbereitete Aktivierung

Der Testzweig enthält `.github/workflows/tramlibern-3.2-refresh.yml`:
ein eigenständiger Aufruf prüft alle Regressionen, lädt aktuelle Originaldaten,
verifiziert sie und veröffentlicht ausschließlich `data/alerts-v2.json` auf
`test/tramlibern-3.2-verified`. Er kompiliert keine Firmware und greift auf kein Gerät zu.
Abgelaufene Daten bleiben gesperrt; unmittelbar vor dem Push erfolgt eine weitere
Zeit- und Strukturprüfung. Gleichzeitige Refresh-Läufe werden serialisiert.
Ein zwischenzeitlich veränderter Testzweig führt zu einem abgewiesenen Push;
es gibt keinen Force-Push und kein automatisches Zusammenführen.

## Noch nicht eingeschaltet

`ci/cloud-scheduler.yml` ist die vollständige vorbereitete Uhr: alle zehn Minuten
(Minute 7, 17, 27, 37, 47, 57) ruft sie ausschließlich diesen Testworkflow auf.
Sie verwendet das vorhandene Secret `OPENTRANSPORT_API_TOKEN`; keine neuen
Zugangsdaten oder laufender Mac sind erforderlich.

GitHub führt Zeitpläne nur vom Default-Zweig aus. Für die Aktivierung wäre deshalb
die ausdrückliche Ausnahme nötig, genau diese eine neue Datei nach
`main/.github/workflows/tramlibern-3.2-clock.yml` zu übernehmen. Der gesamte
Entwicklungscode bleibt im Testzweig. Bisher wurde main nicht verändert.
Quelle: https://docs.github.com/en/actions/reference/workflows-and-actions/events-that-trigger-workflows

Actions-Zeitpläne können verzögert werden. Der Zehn-Minuten-Takt ist keine
Verfügbarkeitsgarantie: bei Verzögerung oder alter Quelle läuft der Feed nach
spätestens 15 Minuten ab. Es wird niemals eine Aktualität vorgetäuscht.
Die Uhr ist durch Deaktivieren ihres Workflows reversibel abschaltbar.

## Fahrtkennungen

Der Testworkflow archiviert zusätzlich eine offizielle JSON-Antwort ausschließlich
zur Diagnose. Nur bei identischem Quellzeitstempel und exakt gleichen übrigen
Selektorfeldern wird geprüft, ob Binärfeld 7 dem JSON-Feld `originalTripId` entspricht.
Anschließend werden exakte Trip-ID, Original-ID, Kalender und Anfangszeit getrennt
ausgewertet. Diese Beobachtung schaltet keinen unbekannten Binärselektor frei.
Unterschiedliche Aufnahmen, zusätzliche Einschränkungen und widersprüchliche
Kennungen liefern ausdrücklich keinen positiven Nachweis.

Die GTFS-RT-Dokumentation nennt für `original_trip_id` Feld 8; ob für GTFS-SA
absichtlich ein anderes Schema verwendet wird, ist noch nicht offiziell bestätigt.
Quelle: https://opentransportdata.swiss/en/cookbook/realtime-prediction-cookbook/gtfs-rt/

Die archivierte offizielle Aufnahme aus Artefakt 12463107076 belegt am identischen
Quellzeitstempel 1792702298 bei allen 38.158 aktiven Fahrtselektoren die exakte
Gleichheit von Binärfeld 7 und JSON-`originalTripId`, einschließlich aller übrigen
Selektorfelder. Ein unveränderter Ausschnitt mit SHA256-Herkunftsnachweisen ist
unter `ci/fixtures/real_trip_pair_20261022.json` als Regressionstest enthalten.
Das ist ein empirischer Schemanachweis für diese Aufnahme, keine allgemeine
Freigabe zukünftiger unbekannter Zusatzbedingungen.

Die aktuellen Binär-/JSON-Abrufe können unterschiedliche Cache-Generationen liefern.
Dann wird ausdrücklich kein identischer Snapshot behauptet. Die JSON-Fahrtkennungen
werden trotzdem eigenständig gegen den statischen Fahrplan diagnostiziert und
mit ihrem eigenen Quellzeitstempel gekennzeichnet. Dieser Pfad veröffentlicht
keine Warnungen. Das offizielle Cookbook beschreibt außerdem Unterschiede zwischen
Kalendertag/Startzeit in GTFS-RT und Betriebstag/Zeit über 24 Uhr in GTFS Static.
Diese werden für GTFS-SA nicht ungeprüft übernommen oder mit Modulo 24 korrigiert.
Quelle: https://opentransportdata.swiss/en/cookbook/timetable-cookbook/gtfs/

Für den dokumentierten Altfall `tripId == originalTripId == ch:1:sjyid:…`
prüft die Diagnose nun alle statischen `original_trip_id`-Kandidaten. Explizites
Datum und Anfangszeit sind Pflicht. Nur genau ein passender Kandidat und vollständig
auflösbare Alternativen liefern eine Zuordnung; eine bekannte widersprüchliche
Trip-ID wird niemals ersetzt. Die Anfangsabfahrt wird auch dann geladen, wenn sie
außerhalb der ausgewählten Berner Haltestellen liegt. Die Auflösung bleibt zunächst
Diagnose: unbekannte Binärfelder werden dadurch nicht für Displaywarnungen zugelassen.
Dokumentation der unterschiedlichen Kennungsräume:
https://opentransportdata.swiss/en/homogenitaet-der-id-innerhalb-der-gtfs-dienste/
