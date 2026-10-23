"""Read-only cross-format evidence. Never changes the warning decoder or IDs."""
from collections import Counter
from alert_decode import decode_alerts, normalize
from probe_protobuf import fields
from trip_instance import verify_instance, seconds


def json_trip_observations(payload):
    stamp = int(payload['header']['timestamp'])
    return [{'alert_id': a['id'], 'selector_index': i, 'selector': s}
            for a in decode_alerts(payload, now=stamp)
            for i, s in enumerate(a['selectors']) if 'tripId' in s]


def compare_trip_encodings(binary, companion):
    from google.transit import gtfs_realtime_pb2 as pb
    from google.protobuf.json_format import MessageToDict
    feed = pb.FeedMessage()
    feed.ParseFromString(binary)
    if not feed.IsInitialized():
        raise ValueError('incomplete binary feed')
    stamp = int(feed.header.timestamp)
    if str(companion.get('header', {}).get('timestamp')) != str(stamp):
        return {'diagnostic_only': True, 'status': 'different_snapshots',
                'binary_timestamp': stamp,
                'json_timestamp': companion.get('header', {}).get('timestamp')}, []
    # Validate envelope, unique entity IDs, periods and incrementality first.
    active = {a['id'] for a in decode_alerts(companion, now=stamp)
              if 'validation_error' not in a}
    entities = {e['id']: e for e in companion['entity']}
    counts = Counter()
    paired = []
    for entity in feed.entity:
        if entity.id not in active or not entity.HasField('alert'):
            continue
        other = entities.get(entity.id, {}).get('alert', {}).get('informedEntity', [])
        if len(other) != len(entity.alert.informed_entity):
            counts['selector_count_mismatch'] += 1
            continue
        for number, selector in enumerate(entity.alert.informed_entity):
            if not selector.HasField('trip'):
                continue
            counts['examined_trip_selectors'] += 1
            raw = list(fields(selector.trip.SerializeToString()))
            extension = [(n, w, v) for n, w, v in raw if n not in (1, 2, 3, 4, 5, 6)]
            for n, w, _ in extension:
                counts[f'extension_field_{n}_wire_{w}'] += 1
            # This is an OBSERVATION of field 7, not a production schema declaration.
            if len(extension) != 1 or extension[0][:2] != (7, 2):
                counts['unsupported_extension_shape'] += 1
                continue
            try:
                value = extension[0][2].decode('utf-8')
            except UnicodeDecodeError:
                counts['non_utf8_extension'] += 1
                continue
            expected = MessageToDict(selector)
            expected['trip']['originalTripId'] = value
            if expected != other[number]:
                counts['cross_format_mismatch'] += 1
                continue
            counts['field7_equals_json_originalTripId'] += 1
            paired.append({'alert_id': entity.id, 'selector_index': number,
                           'selector': normalize(other[number])})
    return {'diagnostic_only': True, 'status': 'same_timestamp',
            'source_timestamp': stamp, 'counts': dict(counts),
            'note': 'Pairwise equality is evidence for this snapshot only; unknown binary constraints remain blocked.'}, paired


def inspect_trip_instances(g, paired):
    counts = Counter()
    samples = {}
    for row in paired:
        selector = row['selector']
        tid = selector.get('tripId')
        static = g.trips.get(tid)
        if set(selector) - {'agencyId', 'routeId', 'routeType', 'stopId', 'directionId',
                            'tripId', 'originalTripId', 'startDate', 'startTime', 'scheduleRelationship'}:
            reason = 'unsupported_selector_constraints'
        elif static is None:
            reason = 'exact_trip_id_absent'
        elif selector.get('originalTripId') != static.get('original_trip_id'):
            reason = 'original_trip_id_conflict'
        else:
            result = verify_instance(g, selector)
            reason = result[1] if result else 'instance_constraints_match'
        counts[reason] += 1
        first = g.first_departures.get(tid)
        if first:
            try:
                if seconds(first[1]) >= 86400:
                    counts['static_departure_at_or_after_24h'] += 1
            except ValueError:
                pass
        if len(samples.setdefault(reason, [])) < 3:
            samples[reason].append(dict(row, static=static, first_departure=first))
    return {'diagnostic_only': True, 'counts': dict(counts), 'samples': samples,
            'note': 'Instance equality does not establish a stop or affected segment. No date/time wrapping or ID replacement.'}
