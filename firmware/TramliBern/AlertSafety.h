#pragma once
#include <stdint.h>
#include <string.h>
// Pure helpers shared by the ESP32 and host boundary tests.
inline bool verifiedFeedTime(int64_t now, int64_t generated, int64_t expires) {
  return now >= 1700000000LL && generated > 0 && generated <= now + 120 &&
         generated >= now - 900 && expires > now && expires <= generated + 900;
}
inline bool verifiedAlertTime(int64_t now, int64_t start, int64_t end, int64_t expires) {
  return start > 0 && start <= now && end > now && end <= expires;
}
inline bool verifiedStationId(const char *id) {
  const char *prefix = "ch:1:sloid:";
  if (!id || strncmp(id, prefix, 11) != 0) return false;
  id += 11;
  if (!*id) return false;
  for (; *id; ++id) if (*id < '0' || *id > '9') return false;
  return true;
}
