"""Read-only safety checks for TramliBern's published alerts.json format.

This module is deliberately separate from the production GitHub Pages workflow.
A route-only alert must never be assigned to a stop without verified route data.
"""
import json
import re

SLOID = re.compile(r"ch:1:sloid:\d+(?::\d+)*\Z")
MAX_BYTES = 22000


def validate_feed(raw, *, now=None):
    """Parse and validate the current {generated, alerts} JSON contract."""
    if isinstance(raw, str):
        raw = raw.encode("utf-8")
    if not isinstance(raw, bytes) or len(raw) > MAX_BYTES:
        raise ValueError("invalid input or feed exceeds ESP32 buffer")
    try:
        feed = json.loads(raw)
    except (UnicodeError, json.JSONDecodeError) as exc:
        raise ValueError("invalid JSON") from exc
    if not isinstance(feed, dict) or type(feed.get("generated")) is not int or feed["generated"] <= 0:
        raise ValueError("invalid generated timestamp")
    if not isinstance(feed.get("alerts"), list):
        raise ValueError("alerts must be a list")
    for alert in feed["alerts"]:
        if not isinstance(alert, dict):
            raise ValueError("alert must be object")
        stops = alert.get("stops")
        if not isinstance(stops, list) or not stops or not all(
            isinstance(s, str) and SLOID.fullmatch(s) for s in stops
        ):
            raise ValueError("alert must have verified SLOID stops")
        if not isinstance(alert.get("text"), str) or not alert["text"].strip():
            raise ValueError("alert must have text")
        if type(alert.get("end")) is not int or alert["end"] < 0:
            raise ValueError("invalid alert end")
    if now is not None and (type(now) is not int or now < 0):
        raise ValueError("invalid check time")
    return feed


def sloid_matches(selector, chosen_stop):
    """Match station SLOID and its platforms, never an unrelated prefix."""
    return selector == chosen_stop or selector.startswith(chosen_stop + ":")


def matching_alerts(feed, chosen_stops, *, now):
    """Stop-specific alerts only; route-only messages cannot leak into output."""
    if type(now) is not int:
        raise ValueError("now must be integer")
    selected = set(chosen_stops)
    if not all(isinstance(s, str) and SLOID.fullmatch(s) for s in selected):
        raise ValueError("selected stops must be SLOIDs")
    return [a for a in feed["alerts"]
            if (not a["end"] or a["end"] >= now)
            and any(sloid_matches(selector, stop)
                    for selector in a["stops"] for stop in selected)]


def verified_route_match(route_ids, selected_route_ids, *, verified_mapping=False):
    """Route IDs are opaque; do not infer service numbers from ID fragments."""
    if not verified_mapping:
        return False
    return bool(set(route_ids) & set(selected_route_ids))
