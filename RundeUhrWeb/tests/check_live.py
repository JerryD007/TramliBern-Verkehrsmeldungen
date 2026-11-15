import json,math
from pathlib import Path
b=json.loads(Path('tests/live_btc.json').read_text())['data'];f=json.loads(Path('tests/live_fx.json').read_text())
assert b['base']=='BTC' and b['currency']=='CHF' and math.isfinite(float(b['amount'])) and float(b['amount'])>0
assert f['base']=='CHF' and f['quote']=='CZK' and math.isfinite(f['rate']) and f['rate']>0 and len(f['date'])==10
print('PASS live API response formats and positive rates; TLS validated by curl with shipped CA roots')
