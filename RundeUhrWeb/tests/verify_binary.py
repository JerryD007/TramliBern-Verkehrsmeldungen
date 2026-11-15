from pathlib import Path
import struct,hashlib
b=Path('build/RundeUhr.ino.bin').read_bytes()
assert len(b)<=0x1e0000
assert b[0]==0xe9 and int.from_bytes(b[12:14],'little')==5
assert b'RUNDEUHR_C3_OTA_LAYOUT_1' in b
assert b[23]==1 and hashlib.sha256(b[:-32]).digest()==b[-32:]
p=Path('RundeUhr/partitions.csv').read_text()
assert '0x9000,  0x5000' in p and p.count('0x1E0000')==2
print('PASS: ESP32-C3 binary, OTA family, appended SHA-256, original NVS/OTA layout')
print('Firmware bytes:',len(b),'OTA capacity:',0x1e0000,'free:',0x1e0000-len(b))
