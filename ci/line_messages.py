"""Line relevance is not evidence that the selected stop is affected."""
from gtfs_alert_segments import evaluate_selector


def line_match(g, selector, selected):
    if not isinstance(selector, dict): return None
    route = selector.get('routeId')
    trip = selector.get('tripId')
    if route is None and isinstance(trip, str):
        route = g.trips.get(trip, {}).get('route_id')
    if not isinstance(route, str) or route not in g.routes: return None
    # Validate every original constraint at its declared stop, when present.
    # Missing evidence, unsupported trip extensions and contradictions stay blocked.
    anchor = selector.get('stopId', selected)
    status, _ = evaluate_selector(g, selector, anchor)
    if status not in ('candidate', 'confirmed_stop'): return None
    status, _ = evaluate_selector(g, {'routeId': route}, selected)
    if status != 'candidate': return None
    label = g.routes[route].get('route_short_name', '').strip()
    if not label: return None
    return {'route_id': route, 'line': label}
