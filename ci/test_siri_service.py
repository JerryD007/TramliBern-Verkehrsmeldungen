import io
import json
from pathlib import Path
import sys
import unittest
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from service.siri.core import parse_stream,response,MAX_AGE
from service.siri.app import Application
from service.siri.acquire import start,refresh

RAW=(Path(__file__).parent/'fixtures/siri/liebegg-20261023.xml').read_bytes()
OP='ch:1:sboid:100626'
LINE='85:827:12'

class SiriTests(unittest.TestCase):
    def parse(self,raw=RAW): return parse_stream(io.BytesIO(raw))
    def result(self,raw=RAW,offset=60,op=OP,line=LINE):
        s=self.parse(raw)
        return response(s,s['source_timestamp']+offset,op,line)
    def test_real_positive(self):
        a=self.result()['alerts']
        self.assertEqual(len(a),1)
        self.assertIn('Linienmeldung 12: Bern, Liebegg',a[0]['text'])
        self.assertEqual(a[0]['reported_direction'],'H')
        self.assertEqual(a[0]['reported_stop_refs'],['ch:1:sloid:90035'])
        self.assertEqual(a[0]['selected_stop_relevance'],'not_verified')
    def test_no_short_number_match(self): self.assertEqual(self.result(line='12')['alerts'],[])
    def test_other_operator(self): self.assertEqual(self.result(op='827')['alerts'],[])
    def test_expired_source(self):
        with self.assertRaises(ValueError): self.result(offset=MAX_AGE)
    def test_future_source(self):
        with self.assertRaises(ValueError): self.result(offset=-121)
    def test_closed(self): self.assertEqual(self.result(RAW.replace(b'published',b'closed'))['alerts'],[])
    def test_bad_period(self): self.assertEqual(self.result(RAW.replace(b'2026-10-26T00:00:00Z',b'2026-10-18T00:00:00Z'))['alerts'],[])
    def test_future_window(self):
        self.assertEqual(self.result(RAW.replace(b'2026-10-16T06:00:00Z',b'2026-10-25T06:00:00Z'))['alerts'],[])
    def test_missing_validity(self):
        self.assertEqual(self.result(RAW.replace(b'ValidityPeriod',b'OtherPeriod'))['alerts'],[])
    def test_trip_scope_not_widened(self):
        self.assertEqual(self.result(RAW.replace(b'>stopPlace<',b'>vehicleJourney<'))['alerts'],[])
    def test_namespace_required(self):
        with self.assertRaises(ValueError): self.parse(RAW.replace(b'http://www.siri.org.uk/siri',b'urn:wrong'))
    def test_duplicate_situation_suppressed(self):
        import xml.etree.ElementTree as E
        root=E.fromstring(RAW); ns={'s':'http://www.siri.org.uk/siri'}
        situations=root.find('.//s:Situations',ns)
        situations.append(E.fromstring(E.tostring(situations[0])))
        self.assertEqual(self.result(E.tostring(root))['alerts'],[])
    def test_dtd_forbidden(self):
        with self.assertRaises(ValueError): self.parse(b'<!DOCTYPE Siri [<!ENTITY x "x">]>'+RAW)
    def test_truncated_xml(self):
        with self.assertRaises(Exception): self.parse(RAW[:-15])
    def test_no_namespace_naive_regex(self):
        self.assertEqual(self.result(RAW.replace(b'ns0:',b'x:').replace(b'xmlns:ns0',b'xmlns:x'))['alerts'][0]['line_ref'],LINE)
    def request(self,app,query='operator_ref='+OP+'&line_ref='+LINE):
        status=[]
        with patch('service.siri.app.time.time',return_value=self.parse()['source_timestamp']+60):
            body=app({'REQUEST_METHOD':'GET','PATH_INFO':'/v1/line-messages','QUERY_STRING':query},lambda s,h:status.append(s))
        return status[0],json.loads(b''.join(body))
    def test_http_positive(self):
        app=Application();app.load(io.BytesIO(RAW))
        status,body=self.request(app)
        self.assertEqual(status,'200 OK');self.assertEqual(len(body['alerts']),1)
    def test_unavailable_not_empty_success(self): self.assertTrue(self.request(Application())[0].startswith('503'))
    def test_invalid_query(self): self.assertTrue(self.request(Application(),'line_ref=12')[0].startswith('400'))
    def test_failed_load_keeps_complete_snapshot(self):
        app=Application();app.load(io.BytesIO(RAW)); prior=app.snapshot
        with self.assertRaises(Exception): app.load(io.BytesIO(RAW[:-10]))
        self.assertIs(app.snapshot,prior)
    def test_stale_cache_not_renewed_by_request(self):
        app=Application();app.load(io.BytesIO(RAW));app.snapshot['source_timestamp']-=MAX_AGE
        self.assertTrue(self.request(app)[0].startswith('503'))
    def test_oversized_situation_skipped_with_diagnostic(self):
        large=RAW.replace(b'<ns0:Source>',b'<ns0:Extra>'+b'x'*2000001+b'</ns0:Extra><ns0:Source>')
        snapshot=self.parse(large)
        self.assertEqual(snapshot['oversized_situations'],1)
        self.assertEqual(snapshot['rows'],[])
    def test_deep_tree_rejected(self):
        with self.assertRaises(ValueError): self.parse(RAW.replace(b'<ns0:Source>',b'<x>'*65+b'</x>'*65+b'<ns0:Source>'))
    def test_unknown_source_rejected(self):
        with self.assertRaises(ValueError): refresh(Application(),'http://localhost/')
    def test_no_high_rate_mirror(self):
        from service.siri.acquire import MIRROR
        with self.assertRaises(ValueError): start(Application(),MIRROR,60)

if __name__=='__main__': unittest.main()
