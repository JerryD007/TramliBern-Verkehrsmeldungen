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


from alert_decode import decode_alerts
from gtfs_alert_segments import read_gtfs, evaluate_selector


def fetch_inputs(token, root):
    session = requests.Session()
    resp = session.get(SA_URL, headers={
        "Authorization": "Bearer " + token,
        "User-Agent": "TramliBern-3.2-ReadOnly-GitHub-Diagnostic",
        "Accept-Encoding": "gzip, deflate",
        "Accept": "application/json",
    }, timeout=120)
    resp.raise_for_status()
    payload = resp.json()
    alerts = decode_alerts(payload)
    Path("reports").mkdir(exist_ok=True)
    Path("reports/real-gtfs-source.json").write_text(json.dumps(payload), encoding="utf-8")
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
                                  "sha256": sha.hexdigest(), "resolved_url":resp.url}


def iter_table(zf, name):
    with zf.open(name) as fp:
        yield from csv.DictReader(io.TextIOWrapper(fp, encoding="utf-8-sig", newline=""))


def stop_matches(selected, stop):
    return stop == selected or stop.startswith(selected + ":")


def crosscheck(alerts, archive_path, metadata):
    g = read_gtfs(archive_path, SELECTED.values())
    selectors = [s for e in alerts for s in e['selectors']]
    ids = {k: {s[k] for s in selectors if isinstance(s.get(k), str)}
           for k in ('routeId','tripId','stopId')}
    compatibility = {k: {'total':len(ids[k]), 'found':len(ids[k] & set(table)),
                         'missing_sample':sorted(ids[k]-set(table))[:40]}
                     for k,table in [('routeId',g.routes),('tripId',g.trips),('stopId',g.stops)]}
    counts = {k:0 for k in ('explicit_stop','route_candidate','not_matched','unverifiable')}
    translation = dict(confirmed_stop='explicit_stop',candidate='route_candidate',
                       no_match='not_matched',unverifiable='unverifiable')
    results=[]
    reasons={}
    selected_evidence={}
    for name, selected in SELECTED.items():
        family=g.family(selected)
        serving={g.trips[t]['route_id'] for t,visits in g.visits.items()
                 if set(visits.values()) & family}
        selected_evidence[name]={'id':selected,'static_name':g.stops.get(selected,{}).get('stop_name'),
            'platforms':sorted(family),'serving_routes':sorted(serving),
            'alert_routes_intersection':sorted(serving & ids['routeId'])}
        cache={}
        for e in alerts:
            for sel in e['selectors']:
                key=json.dumps(sel,sort_keys=True)
                if key not in cache: cache[key]=evaluate_selector(g,sel,selected)
                status,reason=cache[key]
                status=translation[status]
                counts[status]+=1
                reasons[reason]=reasons.get(reason,0)+1
                if status in ('explicit_stop','route_candidate'):
                    results.append(dict(stop=name,status=status,reason=reason,alert_id=e['id'],selector=sel))
    # Export an exact-row subset for reproducible real-data joins, not synthetic evidence.
    output=Path('reports')
    output.mkdir(exist_ok=True)
    selected_ids=set().union(*(g.family(s) for s in SELECTED.values()))
    trip_ids=set(g.visits)
    route_ids={g.trips[t]['route_id'] for t in trip_ids}
    with zipfile.ZipFile(archive_path) as source, zipfile.ZipFile(output/'real-gtfs-selected.zip','w',zipfile.ZIP_DEFLATED) as dest:
        filters={'stops.txt':('stop_id',selected_ids),'trips.txt':('trip_id',trip_ids),
                 'routes.txt':('route_id',route_ids),'stop_times.txt':('stop_id',selected_ids)}
        for name,(key,allowed) in filters.items():
            with source.open(name) as src:
                reader=csv.DictReader(io.TextIOWrapper(src,encoding='utf-8-sig',newline=''))
                with dest.open(name,'w') as target:
                    writer_stream=io.TextIOWrapper(target,encoding='utf-8',newline='')
                    writer=csv.DictWriter(writer_stream,fieldnames=reader.fieldnames)
                    writer.writeheader()
                    for row in reader:
                        if row.get(key) in allowed: writer.writerow(row)
                    writer_stream.flush()
                    writer_stream.detach()
    quality={'examined_selectors':len(selectors), 'distinct_alert_route_ids':len(ids['routeId']),
        'alert_route_ids_found_in_static':compatibility['routeId']['found'],
        'explicit_stop_selectors':sum('stopId' in s for s in selectors),
        'selected_gtfs_stops_found':len(set().union(*(g.family(s) for s in SELECTED.values()))),
        'matched_selected_stop_route_candidates':counts['route_candidate']}
    return dict(source='opentransportdata.swiss',quality=quality,compatibility=compatibility,
        selected_evidence=selected_evidence,reason_counts=reasons,counts=counts,
        checked_at_utc_epoch=int(time.time()),static=metadata,selected_stops=SELECTED,
        active_alert_entities=len(alerts),matches=results,
        note='Diagnostic only. No publication. Unknown IDs and candidate routes are not warnings.')


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
