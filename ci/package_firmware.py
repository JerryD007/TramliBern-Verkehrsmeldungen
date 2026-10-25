"""Package only a successfully compiled sketch; no upload or device access."""
from pathlib import Path
import hashlib,zipfile,os
files={}
for p in Path('reports/firmware').iterdir():
    if p.suffix in ('.ino','.h','.bin'): files['TramliBern/'+p.name]=p.read_bytes()
for name,path in [('UPLOAD-3.2-TEST.md','UPLOAD-3.2-TEST.md'),('memory.md','reports/memory.md'),('board.txt','reports/board.txt')]:
    files[name]=Path(path).read_bytes()
files['BUILD.txt']=('TramliBern 3.2 TEST\nCommit: '+os.environ.get('GITHUB_SHA','local build')+'\n').encode()
files['SHA256SUMS.txt']=''.join(hashlib.sha256(b).hexdigest()+'  '+name+'\n' for name,b in sorted(files.items())).encode()
with zipfile.ZipFile('reports/TramliBern-3.2-TEST.zip','w',zipfile.ZIP_DEFLATED) as z:
    for name,b in files.items():z.writestr(name,b)
