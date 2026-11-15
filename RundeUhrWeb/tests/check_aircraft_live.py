import json,math
from pathlib import Path
d=json.loads(Path('tests/live_aircraft.json').read_text());assert isinstance(d['ac'],list) and isinstance(d['now'],(int,float)) and math.isfinite(d['now']);assert d.get('msg','No error')=='No error'
print('PASS live adsb.fi API schema, TLS validated by curl with shipped roots; not an ESP32 handshake')
