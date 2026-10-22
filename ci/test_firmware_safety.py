import pathlib,subprocess,tempfile,unittest
class FirmwareSafety(unittest.TestCase):
    def test_compiled_boundary_helpers(self):
        root=pathlib.Path(__file__).resolve().parents[1]
        source=r'''
#include "AlertSafety.h"
#include <cassert>
int main() {
 const long long n=1792680000;
 assert(verifiedFeedTime(n,n,n+900));
 assert(!verifiedFeedTime(n,n-901,n+1));
 assert(!verifiedFeedTime(n,n,n));
 assert(!verifiedFeedTime(n,n+121,n+900));
 assert(!verifiedFeedTime(n,n,n+901));
 assert(!verifiedFeedTime(1000,n,n+900));
 assert(verifiedAlertTime(n,n-1,n+1,n+900));
 assert(!verifiedAlertTime(n,n+1,n+100,n+900));
 assert(!verifiedAlertTime(n,n-1,n,n+900));
 assert(!verifiedAlertTime(n,n-1,n+901,n+900));
 assert(verifiedStationId("ch:1:sloid:87480"));
 assert(!verifiedStationId("ch:1:sloid:87480:0:2"));
 assert(!verifiedStationId("ch:1:sloid:"));
 assert(!verifiedStationId("ch:1:sloid:87480x"));
}
'''
        with tempfile.TemporaryDirectory() as d:
            cpp=pathlib.Path(d)/'test.cpp';cpp.write_text(source)
            binary=pathlib.Path(d)/'test'
            subprocess.run(['g++','-std=c++11','-Wall','-Werror','-I',str(root/'firmware/TramliBern'),str(cpp),'-o',str(binary)],check=True)
            subprocess.run([str(binary)],check=True)
