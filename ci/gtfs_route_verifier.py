"""Compatibility API: route membership is evidence for candidates, never warnings."""
import re
from gtfs_alert_segments import read_gtfs, evaluate_selector
SLOID = re.compile(r'ch:1:sloid:\d+(?::\d+)*\Z')

def verified_candidates(gtfs_zip, alert_route_ids, selected_sloids):
    if not isinstance(alert_route_ids,(list,tuple,set)) or not all(isinstance(x,str) and x for x in alert_route_ids):
        raise ValueError('invalid route list')
    if not all(isinstance(s,str) and SLOID.fullmatch(s) for s in selected_sloids):
        raise ValueError('invalid selected SLOID')
    g=read_gtfs(gtfs_zip,selected_sloids)
    return {(r,s) for r in alert_route_ids for s in selected_sloids
            if evaluate_selector(g,{'routeId':r},s)[0]=='candidate'}
