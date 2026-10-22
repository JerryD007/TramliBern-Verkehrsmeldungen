"""Conservative GTFS service-alert selector verification for TramliBern 3.2 TEST.

A route-only alert does not identify an affected stop or route segment.
Only explicitly named stop IDs can confirm stop-specific relevance; optional
route and trip selectors are checked against static GTFS relationships.
Never publish these test decisions directly to the production alerts.json.
"""
import csv
import io
import re
import zipfile

SLOID = re.compile(r"ch:1:sloid:\d+(?::\d+)*\Z")
FILES = ("routes.txt", "trips.txt", "stops.txt", "stop_times.txt")


def read_gtfs(gtfs_zip):
    if not isinstance(gtfs_zip, bytes):
        raise ValueError("GTFS ZIP bytes required")
    with zipfile.ZipFile(io.BytesIO(gtfs_zip)) as archive:
        if any(name not in archive.namelist() for name in FILES):
            raise ValueError("GTFS missing required tables")
        def rows(name):
            return list(csv.DictReader(io.StringIO(archive.read(name).decode("utf-8-sig"))))
        routes, trips, stops, times = (rows(n) for n in FILES)
    route_ids = {r.get("route_id") for r in routes}
    stop_ids = {s.get("stop_id") for s in stops}
    trip_routes = {t["trip_id"]: t["route_id"] for t in trips
                   if t.get("trip_id") and t.get("route_id") in route_ids}
    trip_stops = {}
    for t in times:
        if t.get("trip_id") in trip_routes and t.get("stop_id") in stop_ids:
            trip_stops.setdefault(t["trip_id"], set()).add(t["stop_id"])
    return route_ids, stop_ids, trip_routes, trip_stops


def evaluate_selector(gtfs, selector, selected_stop):
    """Return (status, reason): confirmed_stop, candidate, or no_match.

    confirmation requires an explicit stopId, never a line number guess.
    A route-only/trip-only alert may be a candidate but not a confirmed stop.
    An explicitly named station is confirmed only for that named stop.
    """
    if not isinstance(selector, dict) or not isinstance(selected_stop, str):
        raise ValueError("selector and stop required")
    if not SLOID.fullmatch(selected_stop):
        raise ValueError("selected stop needs SLOID")
    route_ids, stop_ids, trip_routes, trip_stops = gtfs
    route_id = selector.get("routeId")
    trip = selector.get("tripId")
    stop_id = selector.get("stopId")
    if route_id is not None and (not isinstance(route_id, str) or route_id not in route_ids):
        return "no_match", "unknown exact route_id"
    if trip is not None:
        if not isinstance(trip, str) or trip not in trip_routes:
            return "no_match", "unknown exact trip_id"
        if route_id is not None and trip_routes[trip] != route_id:
            return "no_match", "trip belongs to different route"
    if stop_id is not None:
        if not isinstance(stop_id, str) or stop_id not in stop_ids:
            return "no_match", "unknown exact stop_id"
        # Match station and child platform only, never arbitrary ID prefixes.
        if stop_id != selected_stop and not stop_id.startswith(selected_stop + ":"):
            return "no_match", "different stop"
        if trip is not None and stop_id not in trip_stops.get(trip, set()):
            return "no_match", "trip does not visit affected stop"
        if route_id is not None and trip is None:
            # Explicit stop and route: verify route actually serves that stop.
            if not any(trip_routes[t] == route_id and stop_id in trip_stops.get(t, set())
                       for t in trip_routes):
                return "no_match", "route does not serve affected stop"
        return "confirmed_stop", "selector explicitly identifies selected stop"
    if route_id is not None or trip is not None:
        if trip is not None:
            serves = any(s == selected_stop or s.startswith(selected_stop + ":")
                         for s in trip_stops.get(trip, set()))
            return ("candidate", "trip visits stop but no affected segment specified") if serves else ("no_match", "trip does not visit stop")
        serves = any(trip_routes[t] == route_id and any(
            s == selected_stop or s.startswith(selected_stop + ":")
            for s in trip_stops.get(t, set())) for t in trip_routes)
        return ("candidate", "route visits stop but affected segment unknown") if serves else ("no_match", "route does not visit stop")
    return "no_match", "selector has no identifying route, trip or stop"
