import unittest
try:
    from google.transit import gtfs_realtime_pb2 as pb
    from google.protobuf.json_format import MessageToDict
except ImportError:
    pb = None
from gtfs_alert_segments import Index
from trip_diagnostic import compare_trip_encodings, inspect_trip_instances


@unittest.skipIf(pb is None, 'dedicated workflow installs protobuf')
class TripEncodingEvidence(unittest.TestCase):
    def source(self):
        f = pb.FeedMessage()
        f.header.gtfs_realtime_version = '2.0'
        f.header.timestamp = 1000
        e = f.entity.add(id='a')
        s = e.alert.informed_entity.add(stop_id='s')
        s.trip.trip_id = 't'
        s.trip.MergeFromString(b'\x3a\x03org')
        j = MessageToDict(f)
        j['entity'][0]['alert']['informedEntity'][0]['trip']['originalTripId'] = 'org'
        return f.SerializeToString(), j

    def test_paired_fields_are_evidence_only(self):
        raw, j = self.source()
        report, rows = compare_trip_encodings(raw, j)
        self.assertEqual(report['counts']['field7_equals_json_originalTripId'], 1)
        self.assertTrue(report['diagnostic_only'])
        self.assertEqual(rows[0]['selector']['originalTripId'], 'org')

    def test_different_snapshot_does_not_prove_mapping(self):
        raw, j = self.source(); j['header']['timestamp'] = '1001'
        report, rows = compare_trip_encodings(raw, j)
        self.assertEqual(report['status'], 'different_snapshots')
        self.assertEqual(rows, [])

    def test_any_extra_constraint_blocks_pair(self):
        raw, j = self.source()
        j['entity'][0]['alert']['informedEntity'][0]['directionId'] = 0
        report, rows = compare_trip_encodings(raw, j)
        self.assertEqual(rows, [])
        self.assertEqual(report['counts']['cross_format_mismatch'], 1)

    def test_conflicting_original_id_blocks_pair(self):
        raw, j = self.source()
        j['entity'][0]['alert']['informedEntity'][0]['trip']['originalTripId'] = 'wrong'
        self.assertEqual(compare_trip_encodings(raw, j)[1], [])


class TripInstanceEvidence(unittest.TestCase):
    def test_exact_missing_conflicting_and_matching_are_separate(self):
        g = Index(trips={'t': {'original_trip_id': 'org'}})
        rows = [{'selector': {'tripId': t, 'originalTripId': org}}
                for t, org in [('missing', 'org'), ('t', 'wrong'), ('t', 'org')]]
        report = inspect_trip_instances(g, rows)
        self.assertEqual(report['counts'], {'exact_trip_id_absent': 1,
                         'original_trip_id_conflict': 1, 'instance_constraints_match': 1})
        self.assertTrue(report['diagnostic_only'])
