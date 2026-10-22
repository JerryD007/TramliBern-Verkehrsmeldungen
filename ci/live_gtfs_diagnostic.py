"""Read-only live Swiss GTFS-SA vs GTFS-static diagnostic.

Never publishes or alters alerts.json. Requires OPENTRANSPORT_API_TOKEN in
GitHub Actions. Records real route/trip/stop relationships, not textual guesses.
"""
import csv
import hashlib
import io
import json
import os
from pathlib import Path
import re
import tempfile
import time
import zipfile

import requests

SA_URL = "https://api.opentransportdata.swiss/la/gtfs-sa/?format=json"
STATIC_URL = "https://data.opentransportdata.swiss/de/dataset/timetable-2026-gtfs2020/permalink"
# Device screenshots, confirmed IDs; these are selectors to evaluate, not feed mappings.
SELECTED = {
    "Giacomettistrasse": "ch:1:sloid:90090",
    "Ostring": "ch:1:sloid:87480",
    "Zentrum Paul Klee": "ch:1:sloid:90127",
    "Burgernziel": "ch:1:sloid:87481",
    "Brunnadernstrasse": "ch:1:sloid:7061",
}
SLOID = re.compile(r"ch:1:sloid:\d+(?::\d+)*\Z")


def decode_alerts(payload):
    """Extract original selectors, preserving their identity and intent."""
    if not isinstance(payload, dict) or not isinstance(payload.get("entity"), list):
        raise ValueError("Unexpected Swiss GTFS-SA JSON envelope")
    out = []
    now = int(time.time())
    for entity in payload["entity"]:
        if not isinstance(entity, dict) or not isinstance(entity.get("alert"), dict):
            continue
        alert = entity["alert"]
        periods = alert.get("activePeriod") or []
        if periods:
            active = False
            for p in periods:
                if not isinstance(p, dict):
                    continue
                try:
                    start, end = int(p.get("start") or 0), int(p.get("end") or 0)
                except (TypeError, ValueError):
                    continue
                if start <= now and (not end or end >= now):
                    active = True
            if not active:
                continue
        selectors = []
        for s in alert.get("informedEntity") or []:
            if not isinstance(s, dict):
                continue
            # API's JSON is not standardised: preserve original selector keys.
            fields = {k: s.get(k) for k in ("agencyId", "routeId", "tripId", "stopId", "directionId")
                      if s.get(k) is not None}
            trip = s.get("trip")
            if isinstance(trip, dict) and trip.get("tripId"):
                fields["tripId"] = trip["tripId"]
                if trip.get("routeId") and not fields.get("routeId"):
                    fields["routeId"] = trip["routeId"]
                if trip.get("directionId") is not None:
                    fields["directionId"] = trip["directionId"]
            if fields:
                selectors.append(fields)
        if selectors:
            out.append({"id": str(entity.get("id", ""))[:100], "selectors": selectors})
    return out


def fetch_inputs(token, root):
    session = requests.Session()
    resp = session.get(SA_URL, headers={
        "Authorization": "Bearer " + token,
        "User-Agent": "TramliBern-3.2-ReadOnly-GitHub-Diagnostic",
        "Accept-Encoding": "gzip, deflate",
        "Accept": "application/json",
    }, timeout=120)
    resp.raise_for_status()
    alerts = decode_alerts(resp.json())
    # Download to disk: GTFS static is typically >100 MB compressed.
    archive_path = root / "gtfs-static.zip"
    sha = hashlib.sha256()
    size = 0
    with session.get(STATIC_URL, stream=True, timeout=(30, 240), allow_redirects=True) as resp:
        resp.raise_for_status()
        with archive_path.open("wb") as f:
            for chunk in resp.iter_content(1024 * 1024):
                if not chunk:
                    continue
                size += len(chunk)
                if size > 350_000_000:
                    raise ValueError("GTFS archive unexpectedly large")
                f.write(chunk)
                sha.update(chunk)
    return alerts, archive_path, {"source_url": STATIC_URL, "bytes": size,
                                  "sha256": sha.hexdigest()}


def iter_table(zf, name):
    with zf.open(name) as fp:
        yield from csv.DictReader(io.TextIOWrapper(fp, encoding="utf-8-sig", newline=""))


def stop_matches(selected, stop):
    return stop == selected or stop.startswith(selected + ":")


def crosscheck(alerts, archive_path, metadata):
    """Conservative exact-ID join using streaming CSVs; no publication."""
    routes_requested = set()
    trips_requested = set()
    stops_requested = set(SELECTED.values())
    for e in alerts:
        for s in e["selectors"]:
            if isinstance(s.get("routeId"), str):
                routes_requested.add(s["routeId"])
            if isinstance(s.get("tripId"), str):
                trips_requested.add(s["tripId"])
    with zipfile.ZipFile(archive_path) as zf:
        expected = {"routes.txt", "trips.txt", "stop_times.txt", "stops.txt"}
        if not expected.issubset(zf.namelist()):
            raise ValueError("Static GTFS missing required tables")
        known_routes = {r.get("route_id") for r in iter_table(zf, "routes.txt")
                        if r.get("route_id") in routes_requested}
        # Stops can be platform SLOIDs and differ from station IDs.
        stops = {r["stop_id"] for r in iter_table(zf, "stops.txt")
                 if r.get("stop_id") and any(stop_matches(st, r["stop_id"])
                                           for st in stops_requested)}
        trip_routes = {}
        directions = {}
        for row in iter_table(zf, "trips.txt"):
            tid, rid = row.get("trip_id"), row.get("route_id")
            if tid and (rid in known_routes or tid in trips_requested):
                trip_routes[tid] = rid
                directions[tid] = row.get("direction_id")
        trip_stops = {}
        for row in iter_table(zf, "stop_times.txt"):
            tid, stop = row.get("trip_id"), row.get("stop_id")
            if tid in trip_routes and stop in stops:
                trip_stops.setdefault(tid, set()).add(stop)
    results = []
    counts = {"explicit_stop": 0, "route_candidate": 0,
              "not_matched": 0, "unverifiable": 0}
    examined_selectors = sum(len(e["selectors"]) for e in alerts)
    identified_route_ids = sorted({s["routeId"] for e in alerts
        for s in e["selectors"] if isinstance(s.get("routeId"), str)})
    explicit_stop_selectors = sum(1 for e in alerts for s in e["selectors"]
        if isinstance(s.get("stopId"), str))
    # In a zero-hit result, distinguish "none affected" from incompatible IDs.
    route_ids_in_static = len(set(identified_route_ids) & known_routes)
    diagnostic = {
        "examined_selectors": examined_selectors,
        "distinct_alert_route_ids": len(identified_route_ids),
        "alert_route_ids_found_in_static": route_ids_in_static,
        "explicit_stop_selectors": explicit_stop_selectors,
        "selected_gtfs_stops_found": len(stops),
        "matched_selected_stop_route_candidates": 0,
    }
    for name, selected in SELECTED.items():
        for e in alerts:
            for sel in e["selectors"]:
                s, route, trip = sel.get("stopId"), sel.get("routeId"), sel.get("tripId")
                status, reason = "not_matched", "selector does not match selected stop"
                if isinstance(s, str) and stop_matches(selected, s):
                    # Explicitly targeted, but static mapping may use parent station vs platform.
                    if s not in stops:
                        status, reason = "unverifiable", "explicit stop in alert absent from static stops"
                    elif route and route not in known_routes:
                        status, reason = "unverifiable", "routeId not in selected static version"
                    elif trip and trip not in trip_routes:
                        status, reason = "unverifiable", "tripId not in selected static version"
                    elif trip and route and trip_routes[trip] != route:
                        status, reason = "unverifiable", "routeId and tripId conflict"
                    elif trip and s not in trip_stops.get(trip, set()):
                        status, reason = "unverifiable", "trip does not visit explicit stop in static GTFS"
                    elif route and not trip and not any(
                        rid == route and s in trip_stops.get(tid, set())
                        for tid, rid in trip_routes.items()):
                        status, reason = "unverifiable", "route does not visit explicit stop in static GTFS"
                    else:
                        status, reason = "explicit_stop", "alert explicitly targets selected stop"
                elif s is None and (route or trip):
                    # A route can call at a stop while disruption is elsewhere.
                    candidate_trips = ([trip] if trip else
                                       [tid for tid, rid in trip_routes.items() if rid == route])
                    if any(any(stop_matches(selected, st)
                               for st in trip_stops.get(tid, set())) for tid in candidate_trips):
                        status, reason = "route_candidate", "no explicit stop or affected segment"
                if status != "not_matched":
                    counts[status] += 1
                    results.append({"stop": name, "status": status, "reason": reason,
                                    "alert_id": e["id"],
                                    "selector": {k: v for k, v in sel.items()
                                                 if k in ("agencyId", "routeId", "tripId", "stopId", "directionId")}})
    diagnostic["matched_selected_stop_route_candidates"] = counts["route_candidate"]
    return {
        "source": "opentransportdata.swiss",
        "quality": diagnostic,
        "checked_at_utc_epoch": int(time.time()),
        "static": metadata,
        "selected_stops": SELECTED,
        "active_alert_entities": len(alerts),
        "counts": counts,
        "matches": results[:400],
        "note": ("Diagnostic only. route_candidate is NOT a confirmed disruption. "
                 "Static GTFS and live GTFS-SA versions may differ. "
                 "No changes to production alerts.json or ESP32."),
    }


def main():
    token = os.environ.get("OPENTRANSPORT_API_TOKEN", "").strip()
    if not token:
        raise SystemExit("Missing configured GitHub secret OPENTRANSPORT_API_TOKEN")
    root = Path(tempfile.mkdtemp(prefix="tramli-gtfs-"))
    output = Path("reports")
    output.mkdir(exist_ok=True)
    alerts, gtfs, info = fetch_inputs(token, root)
    report = crosscheck(alerts, gtfs, info)
    (output / "real-gtfs-selector-diagnostic.json").write_text(
        json.dumps(report, indent=2, ensure_ascii=False), encoding="utf-8")
    summary = [
        "# TramliBern 3.2 – Live GTFS diagnostic",
        "",
        f"Live alert entities with selectors: {report['active_alert_entities']}",
        f"GTFS archive SHA256: {info['sha256']}",
        f"GTFS archive bytes: {info['bytes']}",
        "",
        "| Classification | Count |", "|---|---:|",
    ]
    summary += [f"| {k} | {v} |" for k, v in report["counts"].items()]
    summary += ["", "## Dataset compatibility checks", "",
        f"Alert selectors checked: {report['quality']['examined_selectors']}",
        f"Distinct route IDs in alert selectors: {report['quality']['distinct_alert_route_ids']}",
        f"Those exact route IDs found in GTFS-static: {report['quality']['alert_route_ids_found_in_static']}",
        f"Selectors with explicit stop ID: {report['quality']['explicit_stop_selectors']}",
        f"Selected GTFS stop IDs found: {report['quality']['selected_gtfs_stops_found']}",
        "", "Route-only candidates are NOT confirmed affected stops.",
                "Static feed compatibility and affected route segments still require verification.",
                "No production data or firmware changed."]
    (output / "real-gtfs-summary.md").write_text("\n".join(summary)+"\n", encoding="utf-8")
    if os.environ.get("GITHUB_STEP_SUMMARY"):
        with open(os.environ["GITHUB_STEP_SUMMARY"], "a", encoding="utf-8") as f:
            f.write("\n".join(summary)+"\n")
    print("\n".join(summary))


if __name__ == "__main__":
    main()
