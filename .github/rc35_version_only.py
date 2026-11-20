from pathlib import Path
import sys, difflib
p=Path(sys.argv[1]); before=p.read_text(encoding="utf-8")
old="TramliBern 4.0 RC34 OTA-VERIFY"
new="TramliBern 4.0 RC35 OTA-TEST"
assert before.count(old)==1, f"Expected one RC34 VERSION; found {before.count(old)}"
after=before.replace(old,new,1)
# Footer text is also a version label and must match the installed firmware.
old_footer="Version 4.0 RC34 OTA-VERIFY"
new_footer="Version 4.0 RC35 OTA-TEST"
assert after.count(old_footer)==1, f"Footer marker count: {after.count(old_footer)}"
after=after.replace(old_footer,new_footer,1)
# Only the version name may change; hardware display, OTA protocol and preferences remain identical.
changes=list(difflib.unified_diff(before.splitlines(),after.splitlines()))
assert len([line for line in changes if line.startswith('-') and not line.startswith('---')])==2
assert len([line for line in changes if line.startswith('+') and not line.startswith('+++')])==2
assert 'otaPendingAddr' in after and 'otaFromHomeWifi' in after
assert 'showWifiQR()' in after and 'setScreenSleep(' in after
p.write_text(after,encoding="utf-8")
print("RC35 version labels only; no other source changes.")
