"""TramliBern 3.2: route-wide alert matches for displayed stop lines.

Route-wide notification means that a *line* has an active alert; it does not
claim that the selected station or segment is affected. Match only authentic
route IDs resolved to official GTFS route_short_name data.
"""
import re

DISPLAY_LINE = re.compile(r"^(?:[TB]\s+)?([A-Za-z0-9]{1,8})$")


def normalize_display_line(value):
    if not isinstance(value, str):
        raise ValueError("display line must be text")
    m = DISPLAY_LINE.fullmatch(value.strip())
    if not m:
        raise ValueError("invalid display line")
    return m.group(1).upper()


def route_wide_matches(selectors, routes_by_id, displayed_lines):
    """Get definite whole-line *advisories* for the displayed route names.

    Explicit stop/trip selectors are excluded: a trip/stop incident is not
    automatically a whole-line incident.
    routes_by_id maps official route_id to its GTFS route_short_name.
    """
    displayed = {normalize_display_line(x) for x in displayed_lines}
    matches = []
    seen = set()
    for selector in selectors:
        if not isinstance(selector, dict):
            continue
        route = selector.get("routeId")
        if not isinstance(route, str):
            continue
        if selector.get("stopId") or selector.get("tripId") or selector.get("trip"):
            continue
        if selector.get("directionId") is not None:
            # Direction-only incidents are not route-wide alerts.
            continue
        official_line = routes_by_id.get(route)
        if not isinstance(official_line, str):
            continue
        try:
            line = normalize_display_line(official_line)
        except ValueError:
            continue
        if line in displayed and (route, line) not in seen:
            matches.append({"route_id": route, "line": line,
                            "label": "Linienmeldung",
                            "stop_impact_confirmed": False})
            seen.add((route, line))
    return matches
