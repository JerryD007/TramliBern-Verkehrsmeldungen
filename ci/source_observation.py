"""HTTP acquisition evidence, never an authority to renew a source timestamp."""
import hashlib

# Deliberate allowlist: do not archive cookies, authorization or arbitrary headers.
SAFE_HEADERS = ('date', 'age', 'last-modified', 'etag', 'cache-control',
                'expires', 'content-type', 'content-length')


def observe_source(data, headers, source_timestamp, fetched_at):
    if type(source_timestamp) is not int or type(fetched_at) is not int:
        raise ValueError('timestamps must be integers')
    normalized = {k.lower(): v for k, v in headers.items()}
    age = fetched_at - source_timestamp
    return {
        'diagnostic_only': True,
        'sha256': hashlib.sha256(data).hexdigest(),
        'bytes': len(data),
        'source_timestamp': source_timestamp,
        'fetched_at': fetched_at,
        'age_at_acquisition_seconds': age,
        'source_clock_status': ('future' if age < -120 else
                                'stale' if age >= 900 else 'within_policy'),
        'http_headers': {k: normalized[k] for k in SAFE_HEADERS if k in normalized},
        'note': 'HTTP Date/Age and retrieval success do not refresh feed content.',
    }
