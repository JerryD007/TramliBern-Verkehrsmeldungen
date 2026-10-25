import unittest
try:
    from google.transit import gtfs_realtime_pb2 as pb
    from protobuf_decode import protobuf_payload
except ImportError:
    pb=None
from alert_decode import decode_alerts
from gtfs_alert_segments import Index,evaluate_selector

@unittest.skipIf(pb is None,'protobuf dependencies installed by dedicated verification workflow')
class ProtobufSafety(unittest.TestCase):
    def feed(self):
        f=pb.FeedMessage();f.header.gtfs_realtime_version='2.0';f.header.timestamp=1000
        e=f.entity.add();e.id='a';s=e.alert.informed_entity.add();s.stop_id='s'
        e.alert.header_text.translation.add(text='Stop moved',language='en')
        return f
    def status(self,f):
        p=protobuf_payload(f.SerializeToString());s=decode_alerts(p,1000)[0]['selectors'][0]
        return evaluate_selector(Index(stops={'s':{}}),s,'s')[0]
    def test_explicit_stop(self):self.assertEqual(self.status(self.feed()),'confirmed_stop')
    def test_unknown_selector_field(self):
        f=self.feed();s=f.entity[0].alert.informed_entity[0];s.MergeFromString(b'\xa0\x06\x01')
        self.assertEqual(self.status(f),'unverifiable')
    def test_unknown_nested_trip_field(self):
        f=self.feed();s=f.entity[0].alert.informed_entity[0];s.trip.trip_id='t';s.trip.MergeFromString(b'\xa0\x06\x01')
        self.assertEqual(self.status(f),'unverifiable')
    def test_unknown_trip_enum(self):
        f=self.feed();f.entity[0].alert.informed_entity[0].trip.MergeFromString(b'\x20\x63')
        self.assertEqual(self.status(f),'unverifiable')
    def test_unknown_alert_field(self):
        f=self.feed();f.entity[0].alert.MergeFromString(b'\xa0\x06\x01');self.assertEqual(self.status(f),'unverifiable')
    def test_unknown_period_field(self):
        f=self.feed();f.entity[0].alert.active_period.add(start=900).MergeFromString(b'\xa0\x06\x01');self.assertEqual(self.status(f),'unverifiable')
    def test_unknown_header_field(self):
        f=self.feed();f.header.MergeFromString(b'\xa0\x06\x01')
        with self.assertRaises(ValueError):protobuf_payload(f.SerializeToString())
    def test_empty_valid_dataset(self):
        f=self.feed();f.ClearField('entity');self.assertEqual(decode_alerts(protobuf_payload(f.SerializeToString()),1000),[])
    def test_stale_still_rejected(self):
        f=self.feed();f.header.timestamp=1
        with self.assertRaises(ValueError):decode_alerts(protobuf_payload(f.SerializeToString()),1000)
    def test_truncated_binary(self):
        with self.assertRaises(Exception):protobuf_payload(b'\x0a\xff')
    def test_missing_required(self):
        with self.assertRaises(ValueError):protobuf_payload(b'')
    def test_direction_and_trip_constraints_preserved(self):
        f=self.feed();s=f.entity[0].alert.informed_entity[0];s.route_id='r';s.direction_id=0
        s.trip.trip_id='t';s.trip.start_date='20261023';s.trip.start_time='25:00:00'
        p=protobuf_payload(f.SerializeToString());s=decode_alerts(p,1000)[0]['selectors'][0]
        self.assertEqual((s['directionId'],s['startDate'],s['startTime']),(0,'20261023','25:00:00'))

    def test_real_swiss_binary_control_and_unknown_trip(self):
        import json
        from pathlib import Path
        data=json.loads((Path(__file__).parent/'fixtures/real_protobuf_20261022.json').read_text())
        f=pb.FeedMessage();f.header.ParseFromString(bytes.fromhex(data['header_hex']))
        for value in data['entities_hex'].values():f.entity.add().ParseFromString(bytes.fromhex(value))
        rows=decode_alerts(protobuf_payload(f.SerializeToString()),int(f.header.timestamp))
        control=next(a for a in rows if a['id']=='c3b86af6-6649-536e-a2cc-fe8249ad546a')
        self.assertEqual(control['selectors'][0]['stopId'],'ch:1:sloid:90035')
        self.assertNotIn('unsupportedProtobufFields',control['selectors'][0])
        trips=[s for a in rows for s in a['selectors'] if 'tripId' in s]
        self.assertTrue(trips)
        self.assertTrue(all(s.get('unsupportedProtobufFields') for s in trips))
