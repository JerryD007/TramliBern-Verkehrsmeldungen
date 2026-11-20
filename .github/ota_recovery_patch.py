from pathlib import Path
import sys
p=Path(sys.argv[1]); s=p.read_text()
tag='constexpr const char *VERSION = "TramliBern 4.0 RC28 HomeCross";'
assert s.count(tag)==1, "RC28 version guard failed"
s=s.replace(tag,'constexpr const char *VERSION = "TramliBern 4.0 RC28 OTA-RECOVERY1";')
target='  otaPassword = prefs.getString("otaPass", "");'
assert s.count(target)==1, "OTA password location changed"
replacement='''  // One-shot local recovery: remove ONLY the forgotten OTA credential.
  // The recovery flag prevents removal of the NEW password on later boots.
  // All other preference namespaces and keys are left unchanged.
  if (!prefs.getBool("otaRecDone1", false)) {
    prefs.remove("otaPass");
    if (prefs.getString("otaPass", "").length() == 0) {
      prefs.putBool("otaRecDone1", true);
    }
  }
  otaPassword = prefs.getString("otaPass", "");'''
s=s.replace(target,replacement)
assert 'prefs.clear(' not in replacement and 'prefs.remove("otaPass")' in replacement
p.write_text(s)
print("RC28 OTA one-shot recovery patch applied")
