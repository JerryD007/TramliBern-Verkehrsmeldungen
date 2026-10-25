"""Snapshot replacement, withdrawal, conflict, expiry, restart and HTTP contract."""
import copy
import io
import sys
import unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from service.siri.core import parse_stream
from service.siri.state import FeedState,FULL_MAX_AGE
from service.siri.app import Application
from test_siri_service import RAW,OP,LINE

S=parse_stream(io.BytesIO(RAW))
NOW=S['source_timestamp']
SID=next(iter(S['entities']))

def source(stamp=NOW,planned=True,version=1,progress='published',present=True):
    s=copy.deepcopy(S);s['source_timestamp']=stamp
    e=s['entities'][SID];e.update(planned=planned,version=version,progress=progress)
    if progress!='published': e['rows']=[]
    if not present: s['entities']={};s['rows']=[]
    return s

class StateTests(unittest.TestCase):
    def make(self,full=None,rt=None):
        s=FeedState();s.ingest('full',full or source(),NOW)
        s.ingest('unplanned',rt or source(present=False),NOW)
        return s
    def alerts(self,state,now=NOW+60): return state.line_response(now,OP,LINE)['alerts']
    def test_planned_present_with_empty_realtime(self): self.assertEqual(len(self.alerts(self.make())),1)
    def test_requires_both_sources(self):
        s=FeedState();s.ingest('full',source(),NOW)
        with self.assertRaises(ValueError): self.alerts(s)
    def test_restart_starts_unavailable(self):
        with self.assertRaises(ValueError): self.alerts(FeedState())
    def test_newer_withdrawal_suppresses_planned(self):
        self.assertEqual(self.alerts(self.make(rt=source(version=2,progress='closed'))),[])
    def test_withdrawal_persists_when_absent_next_unplanned(self):
        s=self.make(rt=source(version=2,progress='closed'))
        s.ingest('unplanned',source(stamp=NOW+60,present=False),NOW+60)
        self.assertEqual(self.alerts(s,NOW+61),[])
    def test_full_replacement_removes_absent_planned(self):
        s=self.make();s.ingest('full',source(stamp=NOW+60,present=False),NOW+60)
        self.assertEqual(self.alerts(s),[])
    def test_unplanned_absence_removes_old_unplanned(self):
        s=self.make(full=source(planned=False),rt=source(planned=False))
        self.assertEqual(len(self.alerts(s)),1)
        s.ingest('unplanned',source(stamp=NOW+60,present=False),NOW+60)
        self.assertEqual(self.alerts(s),[])
    def test_full_closed_unplanned_beats_old_realtime(self):
        s=self.make(full=source(planned=False,version=2,progress='closed'),rt=source(planned=False))
        self.assertEqual(self.alerts(s),[])
    def test_equal_version_conflict_blocks(self):
        s=self.make(rt=source(progress='closed'))
        self.assertEqual(self.alerts(s),[])
    def test_older_version_cannot_revive(self):
        s=self.make(full=source(version=3,progress='closed'),rt=source(version=2))
        self.assertEqual(self.alerts(s),[])
    def test_old_full_can_serve_planned_with_fresh_rt(self):
        s=self.make();s.ingest('unplanned',source(stamp=NOW+3600,present=False),NOW+3600)
        result=s.line_response(NOW+3600,OP,LINE)
        self.assertEqual(len(result['alerts']),1)
        self.assertEqual(result['full_source_timestamp'],NOW)
    def test_stale_rt_blocks_even_valid_planned(self):
        with self.assertRaises(ValueError): self.alerts(self.make(),NOW+900)
    def test_stale_full_blocks_even_fresh_rt(self):
        s=self.make();s.ingest('unplanned',source(stamp=NOW+FULL_MAX_AGE,present=False),NOW+FULL_MAX_AGE)
        with self.assertRaises(ValueError): self.alerts(s,NOW+FULL_MAX_AGE)
    def test_rollback_rejected(self):
        s=self.make()
        with self.assertRaises(ValueError): s.ingest('full',source(stamp=NOW-1),NOW)
    def test_conflicting_same_generation_rejected(self):
        s=self.make()
        with self.assertRaises(ValueError): s.ingest('full',source(version=2),NOW)
    def test_external_mutation_cannot_change_cache(self):
        src=source();s=self.make(full=src);src['entities'].clear()
        self.assertEqual(len(self.alerts(s)),1)
    def test_original_timestamps_not_refreshed(self):
        s=self.make();s.ingest('full',source(),NOW+60)
        self.assertEqual(s.line_response(NOW+60,OP,LINE)['full_source_timestamp'],NOW)
    def test_no_metadata_is_rejected(self):
        s=FeedState();src=source();del src['entities']
        with self.assertRaises(ValueError): s.ingest('full',src,NOW)
    def test_blocked_realtime_suppresses_old_planned(self):
        src=source();src['entities'][SID]['blocked']=True
        self.assertEqual(self.alerts(self.make(rt=src)),[])
    def test_different_operator_not_matched(self):
        self.assertEqual(self.make().line_response(NOW,'other',LINE)['alerts'],[])
    def test_duplicate_marker_suppresses_planned(self):
        src=source(present=False);src['blocked_ids']=[SID]
        self.assertEqual(self.alerts(self.make(rt=src)),[])

    def test_new_full_with_old_version_cannot_revive_withdrawal(self):
        s=self.make(rt=source(version=2,progress='closed'))
        s.ingest('unplanned',source(stamp=NOW+60,present=False),NOW+60)
        s.ingest('full',source(stamp=NOW+60,version=1),NOW+60)
        self.assertEqual(self.alerts(s),[])
    def test_genuinely_newer_full_version_can_reopen(self):
        s=self.make(rt=source(version=2,progress='closed'))
        s.ingest('unplanned',source(stamp=NOW+60,present=False),NOW+60)
        s.ingest('full',source(stamp=NOW+60,version=3),NOW+60)
        self.assertEqual(len(self.alerts(s)),1)
