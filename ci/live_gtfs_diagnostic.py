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


SA_URL = "https://api.opentransportdata.swiss/la/gtfs-sa"
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

CONTROLS = {"Bern, Liebegg":"ch:1:sloid:90035", "Ostermundigen, Rüti":"ch:1:sloid:90052"}


def fetch_inputs(token, root):
    import requests
    session = requests.Session()
    # Download to disk: GTFS static is typically >100 MB compressed.
    archive_path = root / "gtfs-static.zip"
    static_resolved_url = None
    sha = hashlib.sha256()
    size = 0
    with session.get(STATIC_URL, stream=True, timeout=(30, 240), allow_redirects=True) as resp:
        resp.raise_for_status()
        static_resolved_url = resp.url.split("?")[0]
        with archive_path.open("wb") as f:
            for chunk in resp.iter_content(1024 * 1024):
                if not chunk:
                    continue
                size += len(chunk)
                if size > 350_000_000:
                    raise ValueError("GTFS archive unexpectedly large")
                f.write(chunk)
                sha.update(chunk)
    # Fetch realtime AFTER the large static download, so transfer time cannot
    # consume the alert freshness budget. Keep the original source timestamp.
    resp = session.get(SA_URL, headers={
        "Authorization": "Bearer " + token,
        "User-Agent": "TramliBern-3.2-ReadOnly-GitHub-Diagnostic",
        "Accept-Encoding": "gzip, deflate",
        "Accept": "application/octet-stream",
        "Cache-Control": "no-cache",
    }, timeout=120)
    resp.raise_for_status()
    from protobuf_decode import protobuf_payload
    Path("reports").mkdir(exist_ok=True)
    Path("reports/real-gtfs-source.pb").write_bytes(resp.content)
    payload = protobuf_payload(resp.content)
    Path("reports/real-gtfs-source.json").write_text(json.dumps(payload), encoding="utf-8")
    from source_observation import observe_source
    observation = observe_source(resp.content, resp.headers,
                                 int(payload['header']['timestamp']), int(time.time()))
    Path('reports/source-observation.json').write_text(json.dumps(observation, indent=2))
    freshness_error = None
    try:
        alerts = decode_alerts(payload)  # Validate the freshly acquired source against the local clock.
    except ValueError as exc:
        if not str(exc).startswith('stale or future GTFS-SA header:'): raise
        # Preserve useful historical joins, but publication below still enforces
        # the ORIGINAL timestamp against the current clock and must fail closed.
        freshness_error = str(exc)
        alerts = decode_alerts(payload, now=int(payload['header']['timestamp']))
    return alerts, archive_path, {"source_url": STATIC_URL, "bytes": size,
                                  "sha256": sha.hexdigest(), "resolved_url":static_resolved_url, "source_observation":observation, "source_timestamp":int(payload["header"]["timestamp"]), "freshness_error":freshness_error}


def iter_table(zf, name):
    with zf.open(name) as fp:
        yield from csv.DictReader(io.TextIOWrapper(fp, encoding="utf-8-sig", newline=""))


def crosscheck(alerts, archive_path, metadata):
    selectors = [s for e in alerts for s in e['selectors']]
    requested={s['tripId'] for s in selectors if isinstance(s.get('tripId'),str)}
    g = read_gtfs(archive_path, list(SELECTED.values())+list(CONTROLS.values()), requested)
    ids = {k: {s[k] for s in selectors if isinstance(s.get(k), str)}
           for k in ('routeId','tripId','stopId')}
    compatibility = {k: {'total':len(ids[k]), 'found':len(ids[k] & set(table)),
                         'missing_sample':sorted(ids[k]-set(table))[:40]}
                     for k,table in [('routeId',g.routes),('tripId',g.trips),('stopId',g.stops)]}
    counts = {k:0 for k in ('explicit_stop','route_candidate','not_matched','unverifiable')}
    translation = dict(confirmed_stop='explicit_stop',candidate='route_candidate',
                       no_match='not_matched',unverifiable='unverifiable')
    control_matches=[]
    for name,stop in CONTROLS.items():
        for e in alerts:
            for sel in e['selectors']:
                if not sel.get('stopId') or not (g.family(sel['stopId']) & g.family(stop)): continue
                status,reason=evaluate_selector(g,sel,stop)
                control_matches.append(dict(stop=name,selected_id=stop,alert_id=e['id'],status=status,reason=reason,selector=sel))
    results=[]
    reasons={}
    selected_evidence={}
    for name, selected in SELECTED.items():
        family=g.family(selected)
        serving={g.trips[t]['route_id'] for t,visits in g.visits.items()
                 if set(visits.values()) & family}
        selected_evidence[name]={'id':selected,'static_name':g.stops.get(g.resolve_stop(selected),{}).get('stop_name'),
            'resolved_static_id':g.resolve_stop(selected),'platforms':sorted(family),'serving_routes':sorted(serving),
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
    selected_ids=set().union(*(g.family(s) for s in list(SELECTED.values())+list(CONTROLS.values())))
    trip_ids=set(g.visits)
    route_ids={g.trips[t]['route_id'] for t in trip_ids}
    with zipfile.ZipFile(archive_path) as source, zipfile.ZipFile(output/'real-gtfs-selected.zip','w',zipfile.ZIP_DEFLATED) as dest:
        filters={'stops.txt':('stop_id',selected_ids),'trips.txt':('trip_id',trip_ids),
                 'routes.txt':('route_id',route_ids),'stop_times.txt':('stop_id',selected_ids)}
        for name,(key,allowed) in filters.items():
            if name == 'stop_times.txt':
                stream=io.StringIO()
                fields=list(g.time_rows[0]) if g.time_rows else ['trip_id','stop_id','stop_sequence']
                writer=csv.DictWriter(stream,fieldnames=fields)
                writer.writeheader()
                writer.writerows(g.time_rows)
                dest.writestr(name,stream.getvalue())
                continue
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
        'matched_selected_stop_route_candidates':counts['route_candidate'],
        'trip_selectors':sum('tripId' in s for s in selectors),
        'stop_ids_resolved_via_original_stop_id':sum(s not in g.stops and g.resolve_stop(s) is not None for s in ids['stopId'])}
    with zipfile.ZipFile(archive_path) as z:
        metadata['feed_info']=list(iter_table(z,'feed_info.txt')) if 'feed_info.txt' in z.namelist() else []
    return dict(source='opentransportdata.swiss',quality=quality,compatibility=compatibility,
        selected_evidence=selected_evidence,reason_counts=reasons,counts=counts,control_matches=control_matches,
        checked_at_utc_epoch=int(time.time()),static=metadata,selected_stops=SELECTED,
        trip_instance_samples=[dict(trip_id=t,static=g.trips[t],first_departure=g.first_departures.get(t)) for t in sorted(requested & set(g.trips))[:20]],
        active_alert_entities=sum('validation_error' not in a for a in alerts),matches=results,
        rejected_alerts=[a for a in alerts if 'validation_error' in a],
        note='Diagnostic at source time if stale; freshness_error blocks publication. Unknown IDs and candidate routes are not warnings.')


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
        "Historical diagnostic only; publication blocked: " + info["freshness_error"] if info.get("freshness_error") else "Source was fresh at acquisition; publication rechecks current age.",
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
        "", f"Confirmed live control selectors: {sum(m['status']=='confirmed_stop' for m in report['control_matches'])}",
        f"Quarantined invalid alert entities: {len(report['rejected_alerts'])}",
        "", "Route-only candidates are NOT confirmed affected stops.",
                "Static feed compatibility and affected route segments still require verification.",
                "No production data or firmware changed."]
    (output / "real-gtfs-summary.md").write_text("\n".join(summary)+"\n", encoding="utf-8")
    if os.environ.get("GITHUB_STEP_SUMMARY"):
        with open(os.environ["GITHUB_STEP_SUMMARY"], "a", encoding="utf-8") as f:
            f.write("\n".join(summary)+"\n")
    print("\n".join(summary))
    # Publishable test output is separate from the diagnostic and excludes controls.
    from verified_feed import build_feed
    from datetime import datetime
    from zoneinfo import ZoneInfo
    date=datetime.fromtimestamp(info['source_timestamp'],ZoneInfo('Europe/Zurich')).strftime('%Y%m%d')
    feed_info=info.get('feed_info',[])
    if len(feed_info)!=1 or not feed_info[0].get('feed_start_date','99999999')<=date<=feed_info[0].get('feed_end_date',''):
        raise ValueError('static feed validity does not cover source date')
    if any(not e['resolved_static_id'] for e in report['selected_evidence'].values()):
        raise ValueError('a selected station is unresolved')
    feed=build_feed(report,alerts,int(time.time()),info['source_timestamp'])
    (output/'alerts-v2.json').write_text(json.dumps(feed,ensure_ascii=False),encoding='utf-8')



if __name__ == "__main__":
    main()
