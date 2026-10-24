import os
import re
import sys
from pathlib import Path

status = int(sys.argv[1])
log = Path('reports/compile.log').read_text(errors='replace')
text = ['# Arduino ESP32 compile report', '',
        'Build: ' + ('SUCCESS' if status == 0 else f'FAILED (exit {status})'), '',
        'Board: `' + Path('reports/board.txt').read_text().strip() + '`', '',
        '| Memory | Used (bytes) | Limit (bytes) | Free (bytes) | Used (%) |',
        '|---|---:|---:|---:|---:|']
patterns = [
    ('Program flash', r'Sketch uses (\d+) bytes.*?Maximum is (\d+) bytes'),
    ('Static RAM', r'Global variables use (\d+) bytes.*?Maximum is (\d+) bytes'),
]
missing = []
for label, pattern in patterns:
    match = re.search(pattern, log)
    if match:
        used, limit = map(int, match.groups())
        text.append(f'| {label} | {used} | {limit} | {limit-used} | {100*used/limit:.2f} |')
    else:
        missing.append(label)
        text.append(f'| {label} | unavailable | unavailable | unavailable | unavailable |')
text += ['', 'Percentage = used / limit × 100; free = limit − used.', '',
         'Static RAM is the linker allocation for global/static data. Runtime heap, stack, TLS and JSON allocations require measurements on the physical ESP32.',
         'Compilation does not test display, touch, Wi-Fi or live departures.']
report = '\n'.join(text) + '\n'
Path('reports/memory.md').write_text(report)
if os.environ.get('GITHUB_STEP_SUMMARY'):
    with open(os.environ['GITHUB_STEP_SUMMARY'], 'a') as out:
        out.write(report)
print(report)
if status == 0 and missing:
    raise SystemExit('Compilation passed but memory output could not be parsed: ' + ', '.join(missing))
