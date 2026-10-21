"""GTFS route/stop relationship verifier for TramliBern 3.1 TEST.

Only exact route_id and stop_id joins via trips and stop_times count as evidence.
A stop on a route is a *candidate*, not proof that a disruption affects that
stop or direction. Never publish a candidate as a confirmed stop alert.
"""
import csv
import io
import re
import zipfile

SLOID = re.compile(r"ch:1:sloid:\d+(?::\d+)*\Z")
REQUIRED = ("routes.txt", "trips.txt", "stop_times.txt", "stops.txt")


def _rows(archive, filename):
    if filename not in archive.namelist():
        raise ValueError("Missing GTFS " + filename)
    data = archive.read(filename).decode("utf-8-sig")
    return csv.DictReader(io.StringIO(data))


def verified_candidates(gtfs_zip, alert_route_ids, selected_sloids):
    """Return route/SLOID join evidence, never a confirmed disruption."""
    if not isinstance(alert_route_ids, (list, tuple, set)):
        raise ValueError("route list missing")
    if not all(isinstance(x, str) and x for x in alert_route_ids):
        raise ValueError("invalid route ID")
    if not all(isinstance(s, str) and SLOID.fullmatch(s) for s in selected_sloids):
        raise ValueError("invalid selected SLOID")
    with zipfile.ZipFile(io.BytesIO(gtfs_zip)) as archive:
        for f in REQUIRED:
            if f not in archive.namelist():
                raise ValueError("Missing GTFS " + f)
        known_routes = {r.get("route_id") for r in _rows(archive, "routes.txt")}
        requested = set(alert_route_ids) & known_routes
        if not requested:
            return set()
        trips = {r.get("trip_id"): r.get("route_id") for r in _rows(archive, "trips.txt")
                 if r.get("route_id") in requested and r.get("trip_id")}
        known_stops = {r.get("stop_id") for r in _rows(archive, "stops.txt")}
        selected = set(selected_sloids) & known_stops
        if not selected:
            return set()
        found = set()
        for row in _rows(archive, "stop_times.txt"):
            route_id = trips.get(row.get("trip_id"))
            stop_id = row.get("stop_id")
            if route_id and stop_id in selected:
                found.add((route_id, stop_id))
        return found
