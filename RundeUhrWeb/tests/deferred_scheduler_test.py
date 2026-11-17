from pathlib import Path
import subprocess,tempfile
r=Path(__file__).resolve().parents[1]
for name,view,normal in [('Iss',13,30000),('Quake',14,300000),('News',16,5400000)]:
    source=(r/f'RundeUhr/{name}Network.h').read_text()
    tick=source[source.index(f'inline void {name.lower()}Tick'):]
    fetchargs=f'{name}Data&d,const Config&' if name=='Quake' else f'{name}Data&d'
    pre=f'''#include "{name}Model.h"
#include "FinanceModel.h"
#include <cassert>
bool otaActive=false,networkBusy=false,memoryOK=false,fail=false;
uint32_t clockMs=0xfffffff0U,calls=0;
struct {name}Result {{{name}Data data;}};
int {name.lower()}ResultQueue=0;{name}Result output;
uint32_t millis(){{return clockMs;}}
bool financeMemoryReady(Quote&){{return memoryOK;}}
void fetch{name}({fetchargs}){{assert(networkBusy&&!otaActive);calls++;d.http=200;d.valid=true;d.error[0]=0;}}
void xQueueOverwrite(int,{name}Result*r){{output=*r;}}
'''
    post=f'''int main(){{Config c;c.view={view};c.mask=1;
 {name.lower()}Tick(c);assert(calls==0&&!networkBusy&&strstr(output.data.error,"15 s"));
 clockMs+=14999;memoryOK=true;{name.lower()}Tick(c);assert(calls==0);
 clockMs++;{name.lower()}Tick(c);assert(calls==1&&output.data.valid);
 clockMs+={normal}-1;{name.lower()}Tick(c);assert(calls==1);
 clockMs++;memoryOK=false;{name.lower()}Tick(c);assert(calls==1&&output.data.valid);
 clockMs+=15000;otaActive=true;memoryOK=true;{name.lower()}Tick(c);assert(calls==1&&!networkBusy);
 otaActive=false;{name.lower()}Tick(c);assert(calls==2);
 c.view=0;clockMs+={normal};{name.lower()}Tick(c);assert(calls==2);
 c.mask=1UL<<{view};{name.lower()}Tick(c);assert(calls==3);
 puts("PASS {name}: local RAM retry 15s, normal interval retained, timer wrap, OTA, cache, inactive/manual/cycle");}}
'''
    with tempfile.TemporaryDirectory() as t:
        f=Path(t)/'test.cpp';f.write_text(pre+tick+post)
        subprocess.run(['g++','-std=c++17','-I',str(r/'RundeUhr'),str(f),'-o',t+'/test'],check=True)
        subprocess.run([t+'/test'],check=True)
