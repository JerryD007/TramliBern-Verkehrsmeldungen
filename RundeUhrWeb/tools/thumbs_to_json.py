# PNG encoder with no external dependency: deterministic bytes across local/CI builds.
from pathlib import Path
import struct,zlib,base64,json
r=Path(__file__).resolve().parent.parent
def chunk(t,d):return struct.pack('>I',len(d))+t+d+struct.pack('>I',zlib.crc32(t+d)&0xffffffff)
images=[]
for i in range(17):
 b=(r/f'build/thumb_{i}.ppm').read_bytes();header=b'P6\n240 240\n255\n';assert b.startswith(header);rgb=b[len(header):];assert len(rgb)==240*240*3
 raw=b''.join(b'\0'+rgb[y*720:(y+1)*720] for y in range(240))
 # Lossless PNG compression.
 data=zlib.compress(raw,level=9)
 png=b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',240,240,8,2,0,0,0))+chunk(b'IDAT',data)+chunk(b'IEND',b'')
 images.append(base64.b64encode(png).decode())
(r/'web/thumbs.json').write_text(json.dumps(images,separators=(',',':')))
