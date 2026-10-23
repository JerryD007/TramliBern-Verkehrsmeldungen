"""Independent implementation of the Swiss normative PublishingAction subset.

No GTFS-ID rewriting, stop inference, HTML scraping or monitor code copied.
"""
from datetime import datetime
from xml.parsers import expat
from xml.etree.ElementTree import Element, SubElement

NS = 'http://www.siri.org.uk/siri'
XML_LANG = '{http://www.w3.org/XML/1998/namespace}lang'
MAX_BYTES = 128 * 1024 * 1024
MAX_AGE = 900


def path(value):
    return '/'.join('{' + NS + '}' + part for part in value.split('/'))


def text(node, name):
    return (node.findtext(path(name)) or '').strip()


def timestamp(value):
    dt = datetime.fromisoformat(value.replace('Z', '+00:00'))
    if dt.tzinfo is None:
        raise ValueError('timezone required')
    return int(dt.timestamp())


def periods(node, name, required=False):
    result = []
    for p in node.findall(path(name)):
        start, end = timestamp(text(p, 'StartTime')), timestamp(text(p, 'EndTime'))
        if start >= end:
            raise ValueError('invalid period')
        result.append((start, end))
    if required and not result:
        raise ValueError('missing validity')
    return result


def extract(situation):
    """Return official line records, NOT proof of relevance to a selected stop."""
    if text(situation, 'Progress') != 'published':
        return []
    validity = periods(situation, 'ValidityPeriod', True)
    windows = periods(situation, 'PublicationWindow')
    rows = []
    for action in situation.findall(path('PublishingActions/PublishingAction')):
        scope = action.find(path('PublishAtScope'))
        info = action.find(path('PassengerInformationAction'))
        if scope is None or info is None or text(scope, 'ScopeType') not in ('line', 'stopPlace'):
            continue
        # Vehicle-journey and other scopes cannot be silently widened to lines.
        action_windows = periods(info, 'PublicationWindow')
        action_id = text(info, 'ActionRef')
        if not action_id:
            continue
        summary = ''
        for content in info.findall(path('TextualContent')):
            if text(content, 'TextualContentSize') != 'S':
                continue
            for candidate in content.findall(path('SummaryContent/SummaryText')):
                if candidate.get(XML_LANG, '').lower() == 'de':
                    summary = ' '.join(''.join(candidate.itertext()).split())
        if not summary or len(summary) > 1000:
            continue
        for line in scope.findall(path('Affects/Networks/AffectedNetwork/AffectedLine')):
            operator = text(line, 'AffectedOperator/OperatorRef')
            ref, label = text(line, 'LineRef'), text(line, 'PublishedLineName')
            if not operator or not ref or not label:
                continue
            rows.append(dict(situation_id=text(situation, 'SituationNumber'),
                action_id=action_id, operator_ref=operator, line_ref=ref, line=label,
                text=f'Linienmeldung {label}: {summary}', scope='line',
                selected_stop_relevance='not_verified',
                reported_scope=text(scope, 'ScopeType'),
                reported_direction=text(line, 'Direction/DirectionRef') or None,
                reported_stop_refs=[text(p, 'StopPlaceRef') for p in line.findall(path('StopPlaces/AffectedStopPlace'))],
                validity=validity, publication_windows=windows,
                action_windows=action_windows))
    return rows


def parse_stream(stream):
    """Parse 90+ MB XML incrementally; retain one situation at a time.

    Reject DTDs/entities, oversized documents, deep trees, malformed XML and
    ambiguous duplicate situations. Parse the complete document before publishing.
    """
    parser = expat.ParserCreate(namespace_separator='}')
    stack, nodes, rows, times, statuses = [], [], [], [], []
    seen, duplicates = set(), set()
    capture = None
    situation_count = 0
    situation_bytes = 0
    situation_nodes = 0
    skipped = 0
    skip_depth = None

    def expanded(name):
        return '{' + name if '}' in name else name

    def start(name, attrs):
        nonlocal capture, situation_bytes, situation_nodes, skip_depth, skipped
        stack.append(name)
        if len(stack) == 1 and name != NS + '}Siri':
            raise ValueError('wrong root')
        if skip_depth is not None:
            return
        if len(stack) > 64:
            raise ValueError('XML depth limit')
        if name == NS + '}PtSituationElement':
            if nodes:
                raise ValueError('nested situation')
            situation_bytes = 0
            situation_nodes = 1
            nodes.append(Element(expanded(name), {expanded(k): v for k, v in attrs.items()}))
        elif nodes:
            situation_nodes += 1
            if situation_nodes > 20000:
                skip_depth = len(stack) - len(nodes)
                skipped += 1
                duplicates.add(text(nodes[0], 'SituationNumber'))
                nodes.clear()
                return
            nodes.append(SubElement(nodes[-1], expanded(name), {expanded(k): v for k, v in attrs.items()}))
        if name in (NS + '}ResponseTimestamp', NS + '}Status') and len(stack) == 3 and stack[-2] == NS + '}ServiceDelivery':
            capture = [name, '']
        elif name in (NS + '}ResponseTimestamp', NS + '}Status') and len(stack) == 4 and stack[-2] == NS + '}SituationExchangeDelivery':
            capture = [name, '']

    def data(value):
        nonlocal situation_bytes, skip_depth, skipped
        if skip_depth is not None:
            return
        if nodes:
            situation_bytes += len(value)
            if situation_bytes > 2_000_000:
                skip_depth = len(stack) - len(nodes) + 1
                skipped += 1
                duplicates.add(text(nodes[0], 'SituationNumber'))
                nodes.clear()
                return
            nodes[-1].text = (nodes[-1].text or '') + value
        if capture is not None:
            capture[1] += value
            if len(capture[1]) > 200:
                raise ValueError('header size limit')

    def end(name):
        nonlocal capture, situation_count, skip_depth
        if skip_depth is not None:
            if len(stack) == skip_depth:
                skip_depth = None
                situation_count += 1
            stack.pop()
            return
        if capture is not None and name == capture[0]:
            if name.endswith('}ResponseTimestamp'):
                times.append(timestamp(capture[1].strip()))
            else:
                statuses.append(capture[1].strip())
            capture = None
        if nodes:
            node = nodes.pop()
            if not nodes:
                situation_count += 1
                sid = text(node, 'SituationNumber')
                if not sid or situation_count > 10000:
                    raise ValueError('missing ID or situation count limit')
                if sid in seen:
                    duplicates.add(sid)
                seen.add(sid)
                try:
                    rows.extend(extract(node))
                except (ValueError, OverflowError):
                    pass  # Bad periods suppress the entire situation.
                if len(rows) > 20000:
                    raise ValueError('record limit')
        stack.pop()

    def reject(*_):
        raise ValueError('DTD/entity forbidden')

    parser.StartElementHandler, parser.EndElementHandler = start, end
    parser.CharacterDataHandler = data
    parser.StartDoctypeDeclHandler = reject
    parser.EntityDeclHandler = reject
    parser.ExternalEntityRefHandler = reject
    size = 0
    while chunk := stream.read(65536):
        size += len(chunk)
        if size > MAX_BYTES:
            raise ValueError('document size limit')
        parser.Parse(chunk, False)
    parser.Parse(b'', True)
    if not times or any(s != 'true' for s in statuses):
        raise ValueError('invalid source status/timestamp')
    return dict(source_timestamp=min(times), situations=situation_count,
                bytes=size, oversized_situations=skipped, rows=[r for r in rows if r['situation_id'] not in duplicates])


def response(snapshot, now, operator_ref, line_ref):
    source = snapshot['source_timestamp']
    if not now - MAX_AGE < source <= now + 120:
        raise ValueError('source stale or in future')
    selected = []
    for row in snapshot['rows']:
        if (row['operator_ref'], row['line_ref']) != (operator_ref, line_ref):
            continue
        end = source + MAX_AGE
        for key in ('validity', 'publication_windows', 'action_windows'):
            ranges = row[key]
            active = [b for a, b in ranges if a <= now < b]
            if ranges and not active:
                break
            if active:
                end = min(end, max(active))
        else:
            selected.append({**{k:v for k,v in row.items() if k not in ('validity','publication_windows','action_windows')}, 'expires_at':end})
    if len(selected) > 32:
        raise ValueError('response record limit')
    return dict(schema='tramlibern.siri.lines.v1', source_timestamp=source,
                expires_at=source+MAX_AGE, status='ok',
                coverage='supported_publishing_actions_only',
                oversized_situations=snapshot.get('oversized_situations',0), alerts=selected)
