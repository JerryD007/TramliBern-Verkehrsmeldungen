#include <WiFi.h>
#define TRAMLIBERN_VERSION "3.4-WIFI-MENU-DISPLAY-TEST"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <Arduino_GFX_Library.h>
#include <time.h>
#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include <Update.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>

// LCDWIKI E32R40T: ESP32-32E, ST7796, 320 x 480. Display im Querformat.
Arduino_DataBus *bus = new Arduino_ESP32SPI(2, 15, 14, 13, 12, HSPI);
Arduino_GFX *gfx = new Arduino_ST7796(bus, GFX_NOT_DEFINED, 1);
WebServer server(80);
SPIClass touchSPI(HSPI);
XPT2046_Touchscreen touch(33, 36);
bool touchReady = false;
int activePage = 0;
constexpr int STOPS_PER_PAGE = 2;
constexpr int PAGE_COUNT = 3;
Preferences prefs;

constexpr uint16_t TRAM_COLOR_BLACK = 0x0000, TRAM_COLOR_AMBER = 0xFDE0, TRAM_COLOR_WHITE = 0xFFFF;
// Retro-Ambiente; barevna paleta je zachovana.
constexpr uint16_t BUTTON_RED = 0xF800; // pouziva se pouze pro pripadne varovani
constexpr uint16_t BUTTON_BLUE = 0x001F;
constexpr int CITY_BUTTON_X = 12;
constexpr int CITY_BUTTON_Y = 4;
constexpr int CITY_BUTTON_W = 143;
constexpr int CITY_BUTTON_H = 25;
// Die Stadtbezeichnungen auf dem Display sind ASCII, die API-Suche nutzt den Ortsnamen.
constexpr int CITY_COUNT = 13;
const char *const CITY_LABELS[CITY_COUNT] = {
  "BERN", "ZUERICH", "BASEL", "GENF", "LAUSANNE", "LUZERN", "THUN",
  "ST.GALLEN", "WINTERTHUR", "BIEL", "FRIBOURG", "NEUENBURG", "ZUG"
};
const char *const CITY_QUERIES[CITY_COUNT] = {
  "Bern", "Zürich", "Basel", "Genève", "Lausanne", "Luzern", "Thun",
  "St. Gallen", "Winterthur", "Biel/Bienne", "Fribourg", "Neuchâtel", "Zug"
};
int activeCity = 0;

constexpr int PAGE_BUTTON_X = 166;
constexpr int PAGE_BUTTON_Y = 4;
constexpr int PAGE_BUTTON_W = 174;
constexpr int PAGE_BUTTON_H = 25;
constexpr uint32_t REFRESH_MS = 60000;
constexpr int MAX_STOPS = 5;
constexpr int DEPARTURES_PER_STOP = 3;
constexpr int PAGE3_DEPARTURES = 7;
constexpr int MAX_ROWS_PER_PAGE = PAGE3_DEPARTURES;
constexpr int MAX_DEPARTURES = 4 * DEPARTURES_PER_STOP + PAGE3_DEPARTURES;
float batteryVoltage();
bool stopFetchFailed[MAX_STOPS] = {};
bool fetchFailed = false;
bool clockReady = false;
String lastHttpError;
String stopErrors[MAX_STOPS];
uint32_t lastSuccess[MAX_STOPS] = {};
uint32_t lastAttempt[MAX_STOPS] = {};
uint32_t retryDelay[MAX_STOPS] = {15000, 15000, 15000, 15000, 15000};
String stationCache[STOPS_PER_PAGE];
uint32_t screenRevision = 0;
uint32_t touchExitRevision = 0;
bool touchMenuOpen = false;
int touchMode = 0; // 0 Tafel, 1 Tastatur, 2 Trefferliste, 3 Entfernen, 4 Stadtliste
int touchEditIndex = -1;
String touchQuery;
String touchMatches[5];
int touchMatchCount = 0;
String rowCache[MAX_ROWS_PER_PAGE];
String timeCache[MAX_ROWS_PER_PAGE];
String stops[MAX_STOPS];
int stopCount = 0;
String wifiSsid, wifiPassword;
constexpr uint8_t WIFI_PROFILE_MAX = 4;
String profileSsid[WIFI_PROFILE_MAX], profilePassword[WIFI_PROFILE_MAX];
uint8_t profileCount=0, profilePreferred=0, profileTrying=0;
bool wifiAttemptPending=false, fallbackAP=false;
uint32_t wifiAttemptAt=0, wifiNextTryAt=0, wifiSwitchAt=0;
String wifiLastIssue=""; uint32_t wifiFailureCount=0;

void loadWiFiProfiles() {
  profileCount=0;
  String legacyName=prefs.getString("ssid", "");
  if (legacyName.length()) {
    profileSsid[0]=legacyName;
    profilePassword[0]=prefs.getString("pass", "");
    profileCount=1;
  }
  for (uint8_t i=0;i<WIFI_PROFILE_MAX;i++) {
    String ssid=prefs.getString(("w"+String(i)+"s").c_str(), "");
    bool oldBeta=false;
    if(!ssid.length()) {
      ssid=prefs.getString(("wf"+String(i)+"n").c_str(), "");
      oldBeta=true;
    }
    if (!ssid.length()) continue;
    bool duplicate=false;
    for(uint8_t j=0;j<profileCount;j++) if(profileSsid[j]==ssid)duplicate=true;
    if (!duplicate && profileCount<WIFI_PROFILE_MAX) {
      profileSsid[profileCount]=ssid;
      profilePassword[profileCount]=prefs.getString(
        (oldBeta ? "wf"+String(i)+"p" : "w"+String(i)+"p").c_str(), "");
      profileCount++;
    }
  }
  profilePreferred=0;
}
void persistWiFiProfiles() {
  for(uint8_t i=0;i<WIFI_PROFILE_MAX;i++) {
    prefs.putString(("w"+String(i)+"s").c_str(),i<profileCount?profileSsid[i]:"");
    prefs.putString(("w"+String(i)+"p").c_str(),i<profileCount?profilePassword[i]:"");
  }
  // Stary firmware zna pouze ssid/pass: neodstranujeme jeho pristup.
  if(profileCount){
    prefs.putString("ssid",profileSsid[0]);
    prefs.putString("pass",profilePassword[0]);
  }
}
bool saveWiFiProfile(String ssid,String password,bool updatePassword) {
  ssid.trim();
  if(!ssid.length()||ssid.length()>32||password.length()>63)return false;
  int pos=-1;
  for(uint8_t i=0;i<profileCount;i++)if(profileSsid[i]==ssid)pos=i;
  bool isNew = pos<0;
  if(isNew){if(profileCount>=WIFI_PROFILE_MAX)return false;pos=profileCount++;}
  profileSsid[pos]=ssid;
  if(updatePassword || isNew)profilePassword[pos]=password;
  persistWiFiProfiles();
  profilePreferred=pos;
  wifiNextTryAt=0;
  return true;
}
bool deleteWiFiProfile(uint8_t pos) {
  if(pos>=profileCount||profileCount<=1)return false;
  for(uint8_t i=pos;i+1<profileCount;i++) {
    profileSsid[i]=profileSsid[i+1]; profilePassword[i]=profilePassword[i+1];
  }
  --profileCount;
  profileSsid[profileCount]="";profilePassword[profileCount]="";
  profilePreferred=0;persistWiFiProfiles();
  return true;
}
void startWiFiAttempt() {
  if(!profileCount)return;
  profileTrying=profilePreferred%profileCount;
  WiFi.mode(fallbackAP?WIFI_AP_STA:WIFI_STA);
  wifiLastIssue="Zkousim pripojeni k "+profileSsid[profileTrying];
  WiFi.begin(profileSsid[profileTrying].c_str(),profilePassword[profileTrying].c_str());
  wifiAttemptAt=millis();wifiAttemptPending=true;
}
void wifiTick() {
  if(wifiSwitchAt && (int32_t)(millis()-wifiSwitchAt)>=0) {
    wifiSwitchAt=0;
    wifiAttemptPending=false;
    WiFi.disconnect();
    wifiNextTryAt=0;
  }
  if(WiFi.status()==WL_CONNECTED) {
    if(wifiAttemptPending) {
      profilePreferred=profileTrying;
      wifiSsid=profileSsid[profilePreferred];
      wifiPassword=profilePassword[profilePreferred];
      wifiLastIssue="";
    }
    wifiAttemptPending=false;
    if(fallbackAP) {WiFi.softAPdisconnect(true);fallbackAP=false;WiFi.mode(WIFI_STA);}
    return;
  }
  uint32_t now=millis();
  if(wifiAttemptPending) {
    if(now-wifiAttemptAt<6500UL)return;
    wifiAttemptPending=false;
    ++wifiFailureCount;
    wifiLastIssue="Spojeni se nezdarilo, zkousim dalsi Wi-Fi";
    if(profileCount)profilePreferred=(profileTrying+1)%profileCount;
    if(profilePreferred==0 && !fallbackAP) {
      WiFi.mode(WIFI_AP_STA);
      WiFi.softAP("TRAMLI-BERN-2");fallbackAP=true;
    }
    wifiNextTryAt=now+1000UL;
  }
  if(!fallbackAP && !profileCount) {
    WiFi.mode(WIFI_AP_STA); WiFi.softAP("TRAMLI-BERN-2");fallbackAP=true;
  }
  if(profileCount && (int32_t)(now-wifiNextTryAt)>=0) startWiFiAttempt();
}

String statusText = "Startet";
// GPIO27 is driven HIGH by the original board firmware. Verify that it is
// backlight control on the physical board before using screen-off mode.
constexpr int BACKLIGHT_PIN = 27;
bool displaySleeping = false;
bool displayAutoOff = false; // OFF by default, even after first installation
uint16_t displayIdleMinutes = 10;
uint32_t displayLastActivity = 0;
bool wakeTouchWaitRelease = false;
bool brightnessPwmEnabled=false,brightnessPwmAttached=false,nightEnabled=false;
uint8_t dayBrightness=85,nightBrightness=25,nightFrom=22,nightTo=7;
bool batteryWarningEnabled=false;
uint16_t batteryThresholdMv=3500;
uint32_t nextLightCheck=0;
bool isNightNow() {
  if(!nightEnabled || time(nullptr)<1700000000 || nightFrom==nightTo)return false;
  time_t current=time(nullptr);tm ti;localtime_r(&current,&ti);
  int h=ti.tm_hour;
  return nightFrom<nightTo ? (h>=nightFrom&&h<nightTo):(h>=nightFrom||h<nightTo);
}
void applyBacklight() {
  if(!brightnessPwmEnabled) {
    if(brightnessPwmAttached){ledcDetach(BACKLIGHT_PIN);brightnessPwmAttached=false;pinMode(BACKLIGHT_PIN,OUTPUT);}
    digitalWrite(BACKLIGHT_PIN,displaySleeping?LOW:HIGH);return;
  }
  if(!brightnessPwmAttached) {
    if(!ledcAttach(BACKLIGHT_PIN,1000,8)) {
      brightnessPwmEnabled=false;pinMode(BACKLIGHT_PIN,OUTPUT);
      digitalWrite(BACKLIGHT_PIN,displaySleeping?LOW:HIGH);return;
    }
    brightnessPwmAttached=true;
  }
  int level=displaySleeping?0:(isNightNow()?nightBrightness:dayBrightness);
  ledcWrite(BACKLIGHT_PIN,(uint32_t)level*255/100);
}
bool batteryIsLow(float voltage) {
  return batteryWarningEnabled && voltage>=3.0f && voltage<=4.3f
         && (int)(voltage*1000.0f)<batteryThresholdMv;
}
uint32_t otaAllowedUntil = 0;
bool otaUploadOK = false;
bool otaUploadStarted = false;
String otaUploadError;
size_t otaUploadedBytes = 0;
size_t otaExpectedBytes = 0;
uint32_t otaSessionNonce = 0; // Regenerated only by physical display confirmation
bool otaFinishedOK = false;
// Only allow uploading while the user has physically authorized it.
bool otaAllowed() {
  return otaAllowedUntil && (int32_t)(otaAllowedUntil - millis()) > 0;
}
size_t otaMaxBytes() {
  const esp_partition_t *part = esp_ota_get_next_update_partition(NULL);
  return part ? part->size : 0;
}
// Defensive decimal parser: negative, overflow, empty and oversize are invalid.
bool otaParseFirmwareSize(const String &value, size_t &out) {
  out = 0;
  size_t partitionSize = otaMaxBytes();
  if (!partitionSize || !value.length() || value.length() > 9) return false;
  for (size_t i=0; i<value.length(); ++i) {
    char ch = value[i];
    if (ch < '0' || ch > '9') return false;
    out = out*10 + (size_t)(ch-'0');
    if (out > partitionSize) return false;
  }
  return out > 0;
}
bool otaSessionAuthorized() {
  return otaAllowed() && otaSessionNonce != 0 &&
         server.arg("nonce") == String(otaSessionNonce, HEX);
}
void drawBoard();
void displayWake() {
  displaySleeping = false;
  applyBacklight();
  touchMenuOpen = false; touchMode = 0; // Safari wake restores departure board
  displayLastActivity = millis();
  ++touchExitRevision;
  // Keep all application data in RAM; repaint the last selected board.
  drawBoard();
}
void displaySleep() {
  if (displaySleeping) return;
  displaySleeping = true;
  applyBacklight();
}

uint32_t lastRefresh = 0;
struct Departure {
  String line, destination, station;
  time_t when;
};
Departure departures[MAX_DEPARTURES];
int departureCount = 0;


String escapeHtml(const String &s) {
  String out;
  for (size_t i = 0; i < s.length(); ++i) {
    switch (s[i]) {
      case '&': out += "&amp;"; break;
      case '<': out += "&lt;"; break;
      case '>': out += "&gt;"; break;
      case '"': out += "&quot;"; break;
      case '\'': out += "&#39;"; break;
      default: out += s[i];
    }
  }
  return out;
}

String urlEncode(const String &s) {
  const char *hex = "0123456789ABCDEF";
  String out;
  for (size_t i = 0; i < s.length(); ++i) {
    uint8_t c = (uint8_t)s[i];
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
        (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.') out += char(c);
    else { out += '%'; out += hex[c >> 4]; out += hex[c & 15]; }
  }
  return out;
}

// Begrenzter Empfangspuffer: HTTPClient verarbeitet die HTTP-Uebertragung selbst.
// Die API-Feldauswahl reduziert bereits die uebertragene Antwort.
class JsonResponseBuffer : public Stream {
 public:
  static constexpr size_t CAPACITY = 24576;
  char *data;
  size_t length = 0;
  bool overflow = false;
  JsonResponseBuffer() : data((char *)malloc(CAPACITY + 1)) { if (data) data[0] = 0; }
  ~JsonResponseBuffer() { free(data); }
  size_t write(uint8_t value) override { return write(&value, 1); }
  size_t write(const uint8_t *buffer, size_t size) override {
    if (!data || size > CAPACITY - length) { overflow = true; return 0; }
    memcpy(data + length, buffer, size);
    length += size;
    data[length] = 0;
    return size;
  }
  int available() override { return 0; }
  int read() override { return -1; }
  int peek() override { return -1; }
  void flush() override {}
};

bool getJson(const String &url, JsonDocument &doc) {
  lastHttpError = "";
  doc.clear();
  WiFiClientSecure client;
  client.setInsecure(); // Bisherige TLS-Konfiguration.
  HTTPClient http;
  http.setReuse(false);
  http.setTimeout(15000);
  if (!http.begin(client, url)) { lastHttpError = "HTTP Start fehlgeschlagen"; return false; }
  http.addHeader("Accept", "application/json");
  http.addHeader("Accept-Encoding", "identity");
  int code = http.GET();
  int expected = http.getSize();
  Serial.println("URL: " + url);
  if (code != HTTP_CODE_OK) {
    lastHttpError = "HTTP " + String(code) + ": " + http.errorToString(code);
    http.end(); return false;
  }
  if (expected > (int)JsonResponseBuffer::CAPACITY) {
    lastHttpError = "Antwort zu gross: " + String(expected) + " Bytes; Feldfilter pruefen";
    http.end(); return false;
  }
  JsonResponseBuffer response;
  if (!response.data) {
    lastHttpError = "Empfangspuffer nicht verfuegbar; Heap " + String(ESP.getFreeHeap());
    http.end(); return false;
  }
  // Dieser Weg nutzt die HTTPClient-Behandlung fuer Laenge und Chunked-Encoding.
  int received = http.writeToStream(&response);
  http.end();
  Serial.printf("HTTP %d | Transfer %d | Bytes %u | erwartet %d | Heap %u\n",
    code, received, (unsigned)response.length, expected, (unsigned)ESP.getFreeHeap());
  if (received < 0 || response.overflow || (expected >= 0 && response.length != (size_t)expected)) {
    lastHttpError = "Transfer " + String(received) + ": " + http.errorToString(received) +
                   " | gelesen " + String((unsigned)response.length) + " | erwartet " + String(expected);
    return false;
  }
  if (!response.length) { lastHttpError = "HTTP 200, aber Antwort leer"; return false; }
  // const char* erzwingt eigene JSON-Strings: der Empfangspuffer wird danach freigegeben.
  DeserializationError error = deserializeJson(doc, (const char *)response.data, response.length);
  if (error) lastHttpError = "JSON: " + String(error.c_str()) + " | Bytes " + String((unsigned)response.length);
  return !error;
}

// Jede Stadt hat eigene Haltestellen; BERN behaelt die alten gespeicherten Daten.
String cityStopKey(int city, int index) {
  return city == 0 ? "stop" + String(index) : "c" + String(city) + "s" + String(index);
}
String cityCountKey(int city) {
  return city == 0 ? "count" : "c" + String(city) + "n";
}
// Jede Stadt merkt sich ihre zuletzt gewaehlte Seite in Preferences.
String cityPageKey(int city) { return "c" + String(city) + "page"; }
void restoreCityPage() {
  const uint8_t saved = prefs.getUChar(cityPageKey(activeCity).c_str(), 0);
  activePage = saved < PAGE_COUNT ? saved : 0;
}
void selectPage(int page) {
  if (page < 0 || page >= PAGE_COUNT || page == activePage) return;
  activePage = page;
  prefs.putUChar(cityPageKey(activeCity).c_str(), (uint8_t)page);
}
void loadCityStops() {
  stopCount = prefs.getUChar(cityCountKey(activeCity).c_str(), 0);
  if (stopCount > MAX_STOPS) stopCount = MAX_STOPS;
  for (int i = 0; i < MAX_STOPS; ++i)
    stops[i] = (prefs.getBool("fixedSlots", false) || i < stopCount) ? prefs.getString(cityStopKey(activeCity, i).c_str(), "") : "";
  stopCount = 0;
  for (int i = 0; i < MAX_STOPS; ++i) if (stops[i].length()) ++stopCount;
}
void resetStopData() {
  departureCount = 0;
  fetchFailed = false;
  for (int i = 0; i < MAX_STOPS; ++i) {
    stopFetchFailed[i] = false;
    stopErrors[i] = "";
    lastSuccess[i] = 0;
    lastAttempt[i] = 0;
    retryDelay[i] = 15000;
  }
}
void loadSettings() {
  prefs.begin("tramli-bern", false);
  wifiSsid = prefs.getString("ssid", "");
  wifiPassword = prefs.getString("pass", "");
  loadWiFiProfiles();
  brightnessPwmEnabled=prefs.getBool("pwmOptIn",false);
  dayBrightness=constrain((int)prefs.getUChar("dayBri",85),35,100);
  nightBrightness=constrain((int)prefs.getUChar("nightBri",25),15,100);
  nightFrom=constrain((int)prefs.getUChar("nightFrom",22),0,23);
  nightTo=constrain((int)prefs.getUChar("nightTo",7),0,23);
  nightEnabled=prefs.getBool("nightOn",false);
  batteryWarningEnabled=prefs.getBool("batWarnOn",false);
  batteryThresholdMv=constrain((int)prefs.getUShort("batWarnMv",3500),3000,3850);
  applyBacklight();
  displayAutoOff = prefs.getBool("dispAuto", false);
  displayIdleMinutes = prefs.getUShort("dispMin", 10);
  if(displayIdleMinutes < 1 || displayIdleMinutes > 240) displayIdleMinutes=10;
  displaySleeping=false;displayLastActivity=millis();
  activeCity = prefs.getUChar("city", 0);
  if (activeCity < 0 || activeCity >= CITY_COUNT) activeCity = 0;
  loadCityStops();
  restoreCityPage();
}
void saveStops() {
  stopCount = 0;
  for (int i = 0; i < MAX_STOPS; ++i) if (stops[i].length()) ++stopCount;
  prefs.putBool("fixedSlots", true);
  resetStopData();
  prefs.putUChar(cityCountKey(activeCity).c_str(), stopCount);
  for (int i = 0; i < MAX_STOPS; ++i)
    prefs.putString(cityStopKey(activeCity, i).c_str(), stops[i]);
}
// Never silently fall back to a different page when its places are occupied.
int firstFreeSlotOnPage(int page) {
  if (page < 0 || page >= PAGE_COUNT) return -1;
  for (int slot = page * STOPS_PER_PAGE; slot < MAX_STOPS && slot < (page + 1) * STOPS_PER_PAGE; ++slot)
    if (!stops[slot].length()) return slot;
  return -1;
}
void removeStopAt(int index) {
  if (index < 0 || index >= MAX_STOPS) return;
  stops[index] = "";
  saveStops();
  lastRefresh = 0;
}
void drawBoard(); // Der Stadtwechsel benutzt die unveraenderte Anzeige-Funktion.
void changeCity(int city) {
  if (city < 0 || city >= CITY_COUNT || city == activeCity) return;
  activeCity = city;
  prefs.putUChar("city", activeCity);
  loadCityStops();
  restoreCityPage();
  resetStopData();
  lastRefresh = 0;
  statusText = "Lade Abfahrten";
  if (!touchMenuOpen) drawBoard();
}
String citySearchQuery(const String &userQuery) {
  String q = userQuery;
  q.trim();
  String city = CITY_QUERIES[activeCity];
  if (!q.startsWith(city) && !q.startsWith(String(CITY_LABELS[activeCity])))
    q = city + ", " + q;
  return q;
}

void drawLine(const String &s, int x, int y, uint16_t color, int size = 2) {
  gfx->setTextColor(color);
  gfx->setTextSize(size);
  gfx->setCursor(x, y);
  gfx->print(s);
}

String asciiDisplay(String s) {
  s.replace("ä", "ae"); s.replace("ö", "oe"); s.replace("ü", "ue");
  s.replace("Ä", "Ae"); s.replace("Ö", "Oe"); s.replace("Ü", "Ue");
  s.replace("é", "e"); s.replace("è", "e"); s.replace("à", "a");
  return s;
}

String clip(String s, int chars) {
  s = asciiDisplay(s);
  return s.length() > (size_t)chars ? s.substring(0, chars - 1) + "." : s;
}

void drawBusIcon(int x, int y) {
  gfx->drawRoundRect(x, y + 3, 34, 13, 2, TRAM_COLOR_AMBER);
  gfx->drawFastHLine(x + 2, y + 4, 30, TRAM_COLOR_AMBER);
  gfx->drawFastHLine(x + 2, y + 13, 30, TRAM_COLOR_AMBER);
  gfx->drawLine(x, y + 7, x + 2, y + 5, TRAM_COLOR_AMBER);
  gfx->drawLine(x, y + 11, x + 2, y + 13, TRAM_COLOR_AMBER);
  gfx->drawFastHLine(x + 1, y + 11, 5, TRAM_COLOR_AMBER);
  gfx->drawRect(x + 2, y + 6, 5, 5, TRAM_COLOR_AMBER);
  gfx->drawRect(x + 8, y + 6, 7, 5, TRAM_COLOR_AMBER);
  gfx->drawFastVLine(x + 16, y + 5, 9, TRAM_COLOR_AMBER);
  gfx->drawFastVLine(x + 19, y + 5, 9, TRAM_COLOR_AMBER);
  gfx->drawRect(x + 20, y + 6, 8, 5, TRAM_COLOR_AMBER);
  gfx->drawRect(x + 28, y + 6, 3, 5, TRAM_COLOR_AMBER);
  gfx->drawRect(x + 32, y + 6, 1, 5, TRAM_COLOR_AMBER);
  gfx->drawFastHLine(x + 6, y + 11, 9, TRAM_COLOR_AMBER);
  gfx->drawFastHLine(x + 20, y + 11, 8, TRAM_COLOR_AMBER);
  gfx->fillCircle(x + 8, y + 16, 2, TRAM_COLOR_AMBER);
  gfx->fillCircle(x + 25, y + 16, 2, TRAM_COLOR_AMBER);
  gfx->fillCircle(x + 8, y + 16, 1, TRAM_COLOR_BLACK);
  gfx->fillCircle(x + 25, y + 16, 1, TRAM_COLOR_BLACK);
}

void drawTramIcon(int x, int y) {
  gfx->drawRoundRect(x, y + 5, 34, 12, 2, TRAM_COLOR_AMBER);
  gfx->drawFastHLine(x + 3, y + 5, 28, TRAM_COLOR_AMBER);
  gfx->drawLine(x + 14, y + 5, x + 18, y, TRAM_COLOR_AMBER);
  gfx->drawLine(x + 18, y, x + 22, y + 5, TRAM_COLOR_AMBER);
  gfx->drawFastHLine(x + 15, y, 7, TRAM_COLOR_AMBER);
  gfx->drawRect(x + 3, y + 8, 5, 5, TRAM_COLOR_AMBER);
  gfx->drawRect(x + 9, y + 8, 5, 5, TRAM_COLOR_AMBER);
  gfx->drawRect(x + 16, y + 7, 5, 9, TRAM_COLOR_AMBER);
  gfx->drawFastVLine(x + 18, y + 7, 9, TRAM_COLOR_AMBER);
  gfx->drawRect(x + 23, y + 8, 4, 5, TRAM_COLOR_AMBER);
  gfx->drawRect(x + 28, y + 8, 4, 5, TRAM_COLOR_AMBER);
  gfx->fillCircle(x + 6, y + 18, 1, TRAM_COLOR_AMBER);
  gfx->fillCircle(x + 11, y + 18, 1, TRAM_COLOR_AMBER);
  gfx->fillCircle(x + 25, y + 18, 1, TRAM_COLOR_AMBER);
  gfx->fillCircle(x + 30, y + 18, 1, TRAM_COLOR_AMBER);
}

// Warnungen zum Datenabruf sind keine Verkehrsmeldungen.
String warningText() {
  if (WiFi.status() != WL_CONNECTED) return "WARNUNG: WLAN getrennt - keine aktuellen Abfahrten";
  if (fetchFailed) return "Datenabruf gestoert - automatische Wiederholung. Alt = letzter Empfang, max. 3 Min. ";
  if (!clockReady) return "Uhrzeit wird synchronisiert";
  return "";
}

// Text mit schwarzem Zeichenhintergrund ueberschreiben, nicht erst die Flaeche loeschen.
void drawOpaque(String text, int x, int y, int width, uint16_t color) {
  text = asciiDisplay(text);
  if (text.length() > (size_t)width) text = text.substring(0, width);
  while (text.length() < (size_t)width) text += ' ';
  gfx->setTextSize(2);
  gfx->setTextColor(color, TRAM_COLOR_BLACK);
  gfx->setCursor(x, y);
  gfx->print(text);
}

// Puvodni retro vzhled: misto prepinace ABFAHRTEN pouze aktualni mesto.
void drawCityButton() {
  gfx->fillRect(12, 4, 350, 26, TRAM_COLOR_BLACK);
  String name = CITY_LABELS[activeCity];
  gfx->setTextColor(TRAM_COLOR_AMBER, TRAM_COLOR_BLACK);
  int size = name.length() > 13 ? 1 : 2;
  gfx->setTextSize(size);
  gfx->setCursor(16, size == 2 ? 8 : 12);
  gfx->print(name);
}

void drawHeader() {
  if (displaySleeping) return;
  static String previousClock;
  static int previousFill = -2;
  static bool previousLow=false;
  static uint32_t revision = 0, batteryRead = 0;
  bool force = revision != screenRevision;
  revision = screenRevision;
  time_t now = time(nullptr);
  clockReady = now > 1700000000;
  String clock = "--:--";
  if (clockReady) {
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    char uhrzeit[6];
    strftime(uhrzeit, sizeof(uhrzeit), "%H:%M", &timeinfo);
    clock = uhrzeit;
  }
  if (force) { drawCityButton(); }
  if (force || clock != previousClock) {
    drawOpaque(clock, 385, 8, 5, TRAM_COLOR_WHITE);
    previousClock = clock;
  }
  if (force || millis() - batteryRead >= 10000) {
    batteryRead = millis();
    float volt = batteryVoltage();
    int fill = -1;
    if (volt >= 3.0 && volt <= 4.3) {
      int percent = constrain((int)((volt - 3.3) / 0.9 * 100), 0, 100);
      fill = percent * 16 / 100;
    }
    bool low=batteryIsLow(volt);
    if (force || fill != previousFill || low != previousLow) {
      gfx->fillRect(448, 12, 25, 13, TRAM_COLOR_BLACK);
      if (fill >= 0) {
        gfx->drawRect(448,12,20,12,low?BUTTON_RED:TRAM_COLOR_WHITE);
        gfx->fillRect(469, 15, 3, 6, TRAM_COLOR_WHITE);
        if(fill>0)gfx->fillRect(450,14,fill,8,low?BUTTON_RED:TRAM_COLOR_AMBER);
      }
      previousFill=fill;previousLow=low;
    }
  }
}

// Auf Seite 1 und 2 wie bisher: je zwei Haltestellen, drei Fahrten.
// Auf Seite 3: genau eine Haltestelle mit sieben Fahrten.
void drawDepartureRows() {
  if (displaySleeping) return;
  time_t now = time(nullptr);
  const bool lastPage = activePage == 2;
  const int stationsOnPage = lastPage ? 1 : STOPS_PER_PAGE;
  const int rowsPerStation = lastPage ? PAGE3_DEPARTURES : DEPARTURES_PER_STOP;
  const int spacing = lastPage ? 27 : 28;
  for (int position = 0; position < stationsOnPage; ++position) {
    int station = activePage * STOPS_PER_PAGE + position;
    const int headerY = 43 + position * 110;
    const int baseY = 66 + position * 110;
    const int cacheBase = lastPage ? 0 : position * DEPARTURES_PER_STOP;
    if (!stops[station].length()) {
      drawOpaque("Haltestelle noch nicht eingerichtet", 12, headerY, 38, TRAM_COLOR_WHITE);
      for (int slot = 0; slot < rowsPerStation; ++slot) {
        int y = baseY + slot * spacing;
        drawOpaque("", 14, y, 32, TRAM_COLOR_AMBER);
        gfx->fillRect(398, y - 3, 74, 24, TRAM_COLOR_BLACK);
      }
      continue;
    }
    String stationTitle = stops[station];
    stationTitle.replace(String(CITY_QUERIES[activeCity]) + ", ", "");
    stationTitle = clip(stationTitle, stopFetchFailed[station] ? 29 : 38);
    if (stopFetchFailed[station]) stationTitle += " (Fehler)";
    if (stationTitle != stationCache[position]) {
      drawOpaque(stationTitle, 12, headerY, 38, TRAM_COLOR_WHITE);
      stationCache[position] = stationTitle;
    }
    int selected[PAGE3_DEPARTURES];
    int shown = 0;
    for (int i = 0; i < departureCount && shown < rowsPerStation; ++i) {
      if (departures[i].station != stops[station]) continue;
      if (stopFetchFailed[station] && millis() - lastSuccess[station] > 180000) continue;
      if (clockReady && departures[i].when < now - 30) continue;
      selected[shown++] = i;
    }
    for (int slot = 0; slot < rowsPerStation; ++slot) {
      int cache = cacheBase + slot;
      int y = baseY + slot * spacing;
      String content, timing;
      Departure *d = slot < shown ? &departures[selected[slot]] : nullptr;
      if (d) {
        content = d->line + "|" + d->destination;
        if (stopFetchFailed[station]) timing = "alt";
        else if (!clockReady) timing = "--";
        else {
          long minutes = max(0L, (long)((d->when - now + 59) / 60));
          if (minutes == 0) timing = (millis() / 500) % 2 == 0 ?
              (d->line.startsWith("T") ? "ICON_T" : "ICON_B") : "";
          else timing = minutes > 999 ? String("999+") : String(minutes) + "'";
        }
      } else if (slot == 0) {
        content = stopFetchFailed[station] ? "Datenabruf fehlgeschlagen" : "Keine Abfahrten verfuegbar";
      }
      if (content != rowCache[cache]) {
        if (d) {
          if (rowCache[cache].indexOf('|') < 0) drawOpaque("", 14, y, 32, TRAM_COLOR_AMBER);
          drawOpaque(clip(d->line, 6), 14, y, 6, TRAM_COLOR_AMBER);
          drawOpaque(clip(d->destination, 22), 100, y, 24, TRAM_COLOR_AMBER);
        } else {
          drawOpaque(content, 14, y, 32, TRAM_COLOR_WHITE);
        }
        rowCache[cache] = content;
      }
      if (timing != timeCache[cache]) {
        gfx->fillRect(398, y - 3, 74, 24, TRAM_COLOR_BLACK);
        if (timing == "ICON_T") drawTramIcon(400, y - 2);
        else if (timing == "ICON_B") drawBusIcon(400, y - 2);
        else drawOpaque(timing, 400, y, 6, TRAM_COLOR_AMBER);
        timeCache[cache] = timing;
      }
    }
  }
}

void drawBoard() {
  if (touchMenuOpen || displaySleeping) return;
  static String previousLayout;
  String layout = String(activeCity) + ":" + String(WiFi.status() == WL_CONNECTED) + ":" + String(stopCount) + ":" + String(activePage) + ":" + String(touchExitRevision);
  for (int i = 0; i < MAX_STOPS; ++i) layout += "|" + stops[i];
  layout += WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
  if (layout == previousLayout) {
    drawHeader();
    if (WiFi.status() == WL_CONNECTED) drawDepartureRows();
    return;
  }
  previousLayout = layout;
  ++screenRevision;
  for (int i = 0; i < STOPS_PER_PAGE; ++i) stationCache[i] = "";
  for (int i = 0; i < MAX_ROWS_PER_PAGE; ++i) {
    rowCache[i] = "\x01";
    timeCache[i] = "\x01";
  }
  gfx->fillScreen(TRAM_COLOR_BLACK);
  gfx->drawFastHLine(10, 34, 460, TRAM_COLOR_AMBER);
  drawHeader();
  if (WiFi.status() != WL_CONNECTED) {
    drawLine("WLAN nicht verbunden", 12, 60, TRAM_COLOR_WHITE);
    drawLine("Setup: TRAMLI-BERN-2", 12, 93, TRAM_COLOR_AMBER);
    drawLine("Browser: 192.168.4.1", 12, 123, TRAM_COLOR_WHITE);
  } else {
    for (int position = 0; position < (activePage == 2 ? 1 : STOPS_PER_PAGE); ++position) {
      int i = activePage * STOPS_PER_PAGE + position;
      if (!stops[i].length()) continue;
      String name = stops[i];
      name.replace(String(CITY_QUERIES[activeCity]) + ", ", "");
      drawLine(clip(name, 38), 12, 43 + position * 110, TRAM_COLOR_WHITE, 2);
    }
    drawDepartureRows();
  }
  gfx->drawFastHLine(10, 267, 460, TRAM_COLOR_AMBER);
  String address = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
  drawLine("Konfiguration: " + address, 12, 276, TRAM_COLOR_WHITE, 1);
  gfx->fillRoundRect(230, 270, 82, 24, 3, BUTTON_BLUE);
  drawLine("MENU", 245, 274, TRAM_COLOR_WHITE, 2);
  for (int page = 0; page < PAGE_COUNT; ++page) {
    int x = 320 + page * 50;
    gfx->fillRect(x, 270, 46, 24, page == activePage ? TRAM_COLOR_AMBER : TRAM_COLOR_BLACK);
    gfx->drawRect(x, 270, 46, 24, TRAM_COLOR_AMBER);
    drawLine(String(page + 1), x + 17, 274, page == activePage ? TRAM_COLOR_BLACK : TRAM_COLOR_WHITE, 2);
  }
  gfx->drawFastHLine(10, 295, 460, TRAM_COLOR_AMBER);
}

// Dokumentiertes ISO-8601-Feld prognosis.departure in Unixzeit umrechnen.
// Expliziter UTC-Offset vermeidet Probleme bei Sommer-/Winterzeit.
int64_t parseDepartureTime(const char *value) {
  if (!value || !*value) return 0;
  int year, month, day, hour, minute, second;
  if (sscanf(value, "%d-%d-%dT%d:%d:%d", &year, &month, &day, &hour, &minute, &second) != 6) return 0;
  if (strlen(value) < 20 || month < 1 || month > 12 || day < 1 || day > 31) return 0;
  int offsetHour = 0, offsetMinute = 0, offset = 0;
  if (value[19] == '+' || value[19] == '-') {
    if (sscanf(value + 20, "%d:%d", &offsetHour, &offsetMinute) != 2) return 0;
    offset = (offsetHour * 60 + offsetMinute) * 60;
    if (value[19] == '-') offset = -offset;
  } else if (value[19] != 'Z') return 0;
  year -= month <= 2;
  int era = (year >= 0 ? year : year - 399) / 400;
  unsigned yoe = (unsigned)(year - era * 400);
  unsigned doy = (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
  unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  int64_t days = (int64_t)era * 146097 + doe - 719468;
  return days * 86400 + hour * 3600 + minute * 60 + second - offset;
}

// Jeder Halt wird separat aktualisiert. Ein Fehler loescht nicht den letzten Empfang.
void fetchStop(int index) {
  lastAttempt[index] = millis();
  JsonDocument doc;
  String url = "https://transport.opendata.ch/v1/stationboard?station=" +
               urlEncode(stops[index]) + "&limit=" + String(index == 4 ? 14 : 6) +
               "&fields%5B%5D=stationboard/category"
               "&fields%5B%5D=stationboard/number"
               "&fields%5B%5D=stationboard/to"
               "&fields%5B%5D=stationboard/stop/departureTimestamp"
               "&fields%5B%5D=stationboard/stop/prognosis/departure"
               "&fields%5B%5D=stationboard/stop/prognosis/departureTimestamp";
  if (!getJson(url, doc) || !doc["stationboard"].is<JsonArray>()) {
    bool alreadyFailed = stopFetchFailed[index];
    stopFetchFailed[index] = true;
    stopErrors[index] = lastHttpError.length() ? lastHttpError : "Antwort ohne stationboard-Liste";
    retryDelay[index] = alreadyFailed ? min((uint32_t)60000, retryDelay[index] * 2) : 15000;
    // Abstand ab Ende des fehlgeschlagenen Abrufs messen.
    lastAttempt[index] = millis();
    Serial.println(stops[index] + ": " + stopErrors[index]);
    return;
  }
  Departure fresh[PAGE3_DEPARTURES];
  const int wanted = index == 4 ? PAGE3_DEPARTURES : DEPARTURES_PER_STOP;
  int count = 0;
  for (JsonObject row : doc["stationboard"].as<JsonArray>()) {
    if (count >= wanted) break;
    JsonObject stop = row["stop"];
    int64_t timestamp = stop["prognosis"]["departureTimestamp"] | 0LL;
    if (!timestamp) timestamp = parseDepartureTime(stop["prognosis"]["departure"] | "");
    if (!timestamp) timestamp = stop["departureTimestamp"] | 0LL;
    if (!timestamp) continue;
    if (time(nullptr) > 1700000000 && timestamp < (int64_t)time(nullptr) - 30) continue;
    String category = String(row["category"] | "");
    category.toUpperCase();
    if (category.length() && category != "T" && category != "TRAM" &&
        category != "B" && category != "BUS" && category != "NFB" && category != "NFO") continue;
    Departure &d = fresh[count++];
    d.station = stops[index];
    String number = String(row["number"] | "");
    bool isTram = category == "T" || category == "TRAM";
    if (!category.length()) isTram = number == "6" || number == "7" || number == "8" || number == "9";
    d.line = String(isTram ? "T " : "B ") + number;
    d.destination = String(row["to"] | "");
    d.when = (time_t)timestamp;
  }
  int keep = 0;
  for (int i = 0; i < departureCount; ++i) {
    bool stillSelected = false;
    for (int j = 0; j < MAX_STOPS; ++j) if (departures[i].station == stops[j]) stillSelected = true;
    if (stillSelected && departures[i].station != stops[index]) departures[keep++] = departures[i];
  }
  departureCount = keep;
  for (int i = 0; i < count && departureCount < MAX_DEPARTURES; ++i) departures[departureCount++] = fresh[i];
  stopFetchFailed[index] = false;
  stopErrors[index] = "";
  lastSuccess[index] = millis();
  retryDelay[index] = 15000;
}

void updateFetchStatus() {
  fetchFailed = false;
  for (int i = 0; i < MAX_STOPS; ++i) if (stopFetchFailed[i]) fetchFailed = true;
  statusText = fetchFailed ? "Abruf gestoert - automatische Wiederholung; alte Daten markiert" :
               departureCount ? "Aktuell" : "Keine Abfahrten gefunden";
  drawBoard();
}

void refreshBoard() {
  if (WiFi.status() != WL_CONNECTED || !stopCount) { drawBoard(); return; }
  for (int i = activePage * STOPS_PER_PAGE; i < min(MAX_STOPS, (activePage + 1) * STOPS_PER_PAGE); ++i) {
    // Auch regulaere Aktualisierungen beachten den Fehler-Abstand.
    if (stopFetchFailed[i] && millis() - lastAttempt[i] < retryDelay[i]) continue;
    if (stops[i].length()) fetchStop(i);
  }
  updateFetchStatus();
}

void retryFailedStops() {
  if (WiFi.status() != WL_CONNECTED) return;
  for (int i = activePage * STOPS_PER_PAGE; i < min(MAX_STOPS, (activePage + 1) * STOPS_PER_PAGE); ++i) {
    if (stopFetchFailed[i] && millis() - lastAttempt[i] >= retryDelay[i]) {
      if (stops[i].length()) fetchStop(i);
      updateFetchStatus();
      break;
    }
  }
}

// Lokale Bedienoberflaeche: keine Cloud und kein JavaScript erforderlich.
String boardLabel() { return prefs.getString("boardname", "Mein Traemli"); }
String currentAddress() {
  return WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
}
String pageStart(const String &title) {
  String html = R"TRAMLI(<!doctype html><html lang="de-CH"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><meta name="theme-color" content="#12477f"><title>Traemli Schweiz</title><style>
:root{color-scheme:light;--ink:#17324b;--muted:#50657a;--blue:#1558b0;--teal:#086b62;--line:#d4e0ed}
*{box-sizing:border-box}body{margin:0;background:#eef4fb;color:var(--ink);font:20px/1.55 system-ui,-apple-system,BlinkMacSystemFont,"Segoe UI",sans-serif;-webkit-text-size-adjust:100%}header{background:#12477f;color:white;padding:24px 20px;border-bottom:5px solid #57d7c0}header div,main{max-width:880px;margin:auto}header b{font-size:30px;letter-spacing:-.6px}header span{display:block;font-size:14px;font-weight:650;letter-spacing:1.5px;color:#d8eaff;margin-top:4px}main{padding:28px 20px 48px}h1{font-size:clamp(30px,5vw,42px);line-height:1.2;letter-spacing:-1px;margin:8px 0 12px;overflow-wrap:anywhere}h2{font-size:26px;line-height:1.3;margin:0 0 18px}p{margin:12px 0 20px}small,.muted{color:var(--muted)}small{font-size:17px}.card{background:white;border:1px solid var(--line);border-top:5px solid #5a8fd1;border-radius:20px;padding:26px;margin:24px 0;box-shadow:0 5px 18px #17324b08}.card>h2:not(:first-child){border-top:2px solid #e5edf7;padding-top:24px;margin-top:28px}.card:has(form[action='/city']){border-top-color:#1558b0;background:#f7faff}.card:has(form[action='/search']){border-top-color:#098477}.card:has(button[aria-current='page']){border-top-color:#e0a126}button,.button{font:inherit;font-size:20px;line-height:1.3;font-weight:700;background:var(--blue);color:white;border:2px solid transparent;border-radius:12px;padding:14px 20px;min-height:56px;cursor:pointer;text-decoration:none;display:inline-flex;justify-content:center;align-items:center;text-align:center;max-width:100%;overflow-wrap:anywhere;touch-action:manipulation}button:hover,.button:hover{filter:brightness(.94)}button:active,.button:active{transform:translateY(1px)}a{color:#134e9d;text-underline-offset:3px}button:focus-visible,a:focus-visible,input:focus-visible,select:focus-visible,summary:focus-visible{outline:3px solid #172e49;outline-offset:4px}input,select{display:block;font:inherit;font-size:20px;line-height:1.4;color:var(--ink);background:#f8fbff;border:2px solid #aebfd2;border-radius:12px;padding:13px 14px;width:100%;max-width:100%;min-width:0;margin:8px 0 18px;min-height:56px}select{padding-right:28px!important}input:focus,select:focus{border-color:var(--blue);background:white}input::placeholder{color:#617489}label{display:block;font-size:18px;font-weight:700;margin-top:16px}ol,ul{list-style:none;padding:0;margin:0}li{display:flex;align-items:flex-start;justify-content:space-between;flex-wrap:wrap;gap:16px;padding:22px 0;border-bottom:1px solid var(--line);overflow-wrap:anywhere}li:last-child{border:0}li>div{width:100%;min-width:0}li>span{flex:1 1 220px}li>form{flex:1 1 240px;min-width:0}li b{font-size:22px}li form+form{margin-top:12px}.subtle{background:#e9f1fc;color:#174f91;border-color:#c4d7f0;font-size:19px;padding:13px 18px}.row{display:flex;gap:12px;flex-wrap:wrap;align-items:stretch}.row form{margin:0;flex:1 1 140px}.row button{width:100%}nav{display:flex;gap:10px;margin:24px 0;flex-wrap:wrap}nav a{flex:1 1 180px;min-height:56px;padding:13px 16px;background:white;border:2px solid #bdcfe4;border-radius:12px;text-align:center;text-decoration:none;font-size:19px;font-weight:700;color:#174f91}nav a:hover{background:#e5effc}.status{padding:16px 20px;background:#e3f4ef;color:#175447;border-left:5px solid #138473;border-radius:12px;font-size:18px}.hint{font-size:17px;color:var(--muted);line-height:1.6}.notice{background:#fff2d4;color:#684600;border-left:5px solid #d99b22;padding:17px 20px;border-radius:12px}.notice:empty{display:none}summary{cursor:pointer;font-weight:700;min-height:52px;padding:10px 0}footer{font-size:16px;color:var(--muted);margin-top:32px;padding-top:20px;border-top:1px solid var(--line)}.address{font-family:ui-monospace,monospace;font-size:18px;display:block;color:var(--muted);margin-top:6px;overflow-wrap:anywhere}.full{width:100%}form{margin:0}button[aria-current='page']{background:#12477f;box-shadow:0 0 0 3px #bdd5f4}form[action='/add'] button,form[action='/search'] button,form[action='/refresh'] button{background:var(--teal);color:white}form[action='/remove'] button,form[action='/board-remove'] button{background:#fff0f0;border-color:#edbebe;color:#a52626}form[action='/move'] button{background:#eaf1fc;color:#174f91}form[action='/page']+ol{margin-top:12px}@media(max-width:520px){body{font-size:19px}header{padding:18px 16px}main{padding:22px 14px 36px}.card{padding:20px 16px;margin:20px 0}h2{font-size:24px}.row{gap:8px}.row form{flex:1 1 80px}.row button{font-size:18px;padding:13px 7px}nav{gap:8px}nav a{flex:1 1 100%;text-align:left}li{display:block}li>form{margin-top:16px}li b{font-size:21px}button,.button{width:100%}header b{font-size:28px}}
</style></head><body><header><div><b>▤ Trämli Bern</b><span>DEINE ABFAHRTSTAFELN</span></div></header><main>)TRAMLI";
  html += "<h1>" + escapeHtml(title) + "</h1><p class='muted'>" + escapeHtml(boardLabel()) + " · " + escapeHtml(currentAddress()) + "</p>";
  html += "<nav aria-label='Navigation'><a href='/'>Haltestellen</a><a href='/boards'>Meine Boards</a><a href='/settings'>Einstellungen</a><a href='/system'>System</a></nav>";
  html += "<form method='post' action='/display/wake'><button class='subtle full'>Probudit displej</button></form>";
  return html;
}
String pageEnd() { return "<footer>TramliBern · Lokalni ovladani · Verze " TRAMLIBERN_VERSION " · Seite 1-2: je 2 Haltestellen / 3 Abfahrten · Seite 3: 1 Haltestelle / 7 Abfahrten</footer></main></body></html>"; }
void sendPage(String html, int code = 200) {
  server.sendHeader("Cache-Control", "no-store");
  server.send(code, "text/html; charset=utf-8", html + pageEnd());
}
void showMessage(const String &message, int code = 400) {
  sendPage(pageStart("Hinweis") + "<section class='card'><p>" + escapeHtml(message) + "</p><a class='button' href='/'>Zurück zur Bedienung</a></section>", code);
}
void redirectHome() { server.sendHeader("Location", "/"); server.send(303); }
void redirectBoards() { server.sendHeader("Location", "/boards"); server.send(303); }
// Nur lokale IPv4-Adressen oder einfache Hostnamen, keine Pfade/Anmeldedaten.
String normalizeBoardAddress(String address) {
  address.trim();
  if (address.startsWith("http://")) address = address.substring(7);
  if (address.endsWith("/")) address.remove(address.length() - 1);
  if (!address.length() || address.length() > 100) return "";
  for (size_t i = 0; i < address.length(); ++i) {
    char c = address[i];
    if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
          (c >= '0' && c <= '9') || c == '.' || c == '-')) return "";
  }
  return "http://" + address + "/";
}
bool readIndex(int limit, int &index) {
  if (!server.hasArg("index")) return false;
  String value = server.arg("index");
  if (!value.length() || value.length() > 2) return false;
  for (size_t i = 0; i < value.length(); ++i) if (value[i] < '0' || value[i] > '9') return false;
  index = value.toInt();
  return index >= 0 && index < limit;
}

void setupRoutes() {
  // Lokalni webova aplikace: pouze odjezdy, zadne GTFS vystrahy.
  server.on("/diagnose", HTTP_GET, []() {
    String html=pageStart("Diagnostika odjezdu");
    html += "<section class='card'><h2>Pripojeni</h2><p>Wi-Fi: <b>"+
       String(WiFi.status()==WL_CONNECTED?"Pripojeno":"Odpojeno")+"</b></p>";
    if(WiFi.status()==WL_CONNECTED){
      html += "<p>SSID: "+escapeHtml(WiFi.SSID())+"</p>";
      html += "<p>IP: "+WiFi.localIP().toString()+"</p>";
    }
    html += "<p>Firmware: <b>" TRAMLIBERN_VERSION "</b></p>";
    html += "<p>Ulozene Wi-Fi: "+String(profileCount)+" / 4, neuspesne pokusy: "+String(wifiFailureCount)+"</p>";
    html += "<p>Posledni stav Wi-Fi: "+escapeHtml(wifiLastIssue)+"</p>";
    html += "<p>Orientacni baterie: "+String(batteryVoltage(),2)+" V</p>";
    html += "<p>Volna RAM: "+String(ESP.getFreeHeap())+" B</p>";
    html += "<p>Stav: "+escapeHtml(statusText)+"</p></section>";
    html += "<section class='card'><h2>Zastavky a cerstvost dat</h2>";
    bool any=false;
    for(int i=0;i<MAX_STOPS;i++){
      if(!stops[i].length())continue;
      any=true;
      html += "<div style='padding:14px 0;border-bottom:1px solid #ddd;overflow-wrap:anywhere'>";
      html += "<p><b>"+escapeHtml(stops[i])+"</b>, strana "+String(i/2+1)+"</p>";
      if(lastSuccess[i]){
        uint32_t secs=(millis()-lastSuccess[i])/1000UL;
        html += "<p>Posledni uspesne nacteni: pred "+String(secs<60?secs:secs/60)+
          String(secs<60?" sekundami":" minutami")+"</p>";
      } else html += "<p>Od restartu zatim nebyla prijata data.</p>";
      if(stopFetchFailed[i]){
        String problem=stopErrors[i];
        if(problem.indexOf("HTTP -")>=0)problem="Chyba spojeni nebo vyprsel cas";
        else if(problem.indexOf("JSON")>=0)problem="Chybna odpoved serveru";
        else if(problem.indexOf("HTTP 5")>=0)problem="Server docasne nedostupny";
        html += "<p class='notice'>Chyba: "+escapeHtml(problem)+"</p>";
      } else if(lastSuccess[i])html += "<p>Posledni stazeni bylo uspesne.</p>";
      html += "</div>";
    }
    if(!any)html += "<p>Zatim nejsou vybrane zastavky.</p>";
    html += "<p class='hint'>Cas meri dobu od stazeni odpovedi, nikoli presnost odjezdu dopravce.</p></section>";
    sendPage(html);
  });
  server.on("/", HTTP_GET, []() {
    String html = pageStart("Meine Haltestellen");
    html += "<section class='card'><h2>Seite frei wählen</h2><div class='row'>";
    for (int page = 0; page < PAGE_COUNT; ++page) {
      html += "<form action='/page' method='post'><input type='hidden' name='page' value='" + String(page) + "'><button" + String(page == activePage ? " aria-current='page'" : " class='subtle'") + ">Seite " + String(page + 1) + (page == activePage ? " ✓" : "") + "</button></form>";
    }
    html += "</div><p class='hint'>Auch leere Seiten sind direkt auswählbar. Unten einen freien Platz einrichten. Jede Stadt merkt sich ihre Seite.</p></section>";
    html += "<section class='card'><h2>Stadt auswählen</h2><p>Aktuelle Stadt: <b>" + String(CITY_LABELS[activeCity]) + "</b></p>";
    html += "<form action='/city' method='post'><label for='city'>Stadt</label><select id='city' name='city' style='width:100%;padding:12px;margin:7px 0 15px'>";
    for (int i = 0; i < CITY_COUNT; ++i)
      html += "<option value='" + String(i) + "'" + (activeCity == i ? " selected" : "") + ">" + escapeHtml(CITY_QUERIES[i]) + "</option>";
    html += "</select><button style='background:#175ac8'>Stadt wechseln</button></form>";
    html += "<p class='hint'>Jede Stadt speichert ihre eigenen Haltestellen. Deine Berner Haltestellen bleiben erhalten.</p></section>";
    for (int i = 0; i < MAX_STOPS; ++i) {
      if (stopFetchFailed[i]) html += "<p class='notice'>" + escapeHtml(stops[i]) + ": " + escapeHtml(stopErrors[i]) + "</p>";
    }
    html += "<p class='status'>Anzeige: " + escapeHtml(statusText) + "</p><p class='notice'>" + escapeHtml(warningText()) + "</p>";
    html += "<section class='card'><h2>Displayseiten verwalten</h2><p class='hint'>Seite 1 und 2: je zwei Haltestellen mit drei Abfahrten. Seite 3: eine Haltestelle mit sieben Abfahrten. Die Position einer Haltestelle waehlen und auf Verschieben tippen. Falls der Platz belegt ist, werden die beiden Haltestellen getauscht.</p>";
    for (int page = 0; page < PAGE_COUNT; ++page) {
      html += "<h2>Seite " + String(page + 1) + (activePage == page ? " (aktiv)" : "") + "</h2>";
      html += "<form action='/page' method='post'><input type='hidden' name='page' value='" + String(page) + "'><button class='subtle'>Diese Seite anzeigen</button></form><ol>";
      for (int pos = 0; pos < (page == 2 ? 1 : STOPS_PER_PAGE); ++pos) {
        int index = page * STOPS_PER_PAGE + pos;
        if (!stops[index].length()) {
          html += "<li><span>Platz " + String(pos + 1) + ": Frei</span></li>";
          continue;
        }
        html += "<li><div><b>" + escapeHtml(stops[index]) + "</b><small class='muted'> · " + String(page == 2 ? 7 : 3) + " Abfahrten</small>";
        html += "<form action='/move' method='post'><input type='hidden' name='index' value='" + String(index) + "'>";
        html += "<label>Position</label><select name='slot' style='width:100%;padding:10px;margin:7px 0'>";
        for (int target = 0; target < MAX_STOPS; ++target) {
          html += "<option value='" + String(target) + "'" + (index == target ? " selected" : "") + ">Seite " + String(target / STOPS_PER_PAGE + 1) + ", Platz " + String(target % STOPS_PER_PAGE + 1) + "</option>";
        }
        html += "</select><button class='subtle'>Verschieben / Tauschen</button></form>";
        html += "<form action='/remove' method='post'><input type='hidden' name='index' value='" + String(index) + "'><button class='subtle'>Entfernen</button></form></div></li>";
      }
      html += "</ol>";
    }
    if (!stopCount) html += "<p>Noch keine Haltestelle ausgewaehlt.</p>";
    html += "<p class='hint'>" + String(stopCount) + " von 5 Haltestellen ausgewaehlt. Jeder Platz ist frei waehlbar. Leere Seiten bleiben leer.</p>";
    html += "<form action='/refresh' method='post'><button class='subtle'>Anzeige aktualisieren</button></form></section>";
    html += "<section class='card'><h2>Haltestelle hinzufügen</h2>";
    if (stopCount >= MAX_STOPS) html += "<p class='notice'>Alle fünf Plätze sind belegt. Entferne zuerst eine Haltestelle, um eine andere hinzuzufügen.</p>";
    else {
      int target = firstFreeSlotOnPage(activePage);
      html += "<form action='/search' method='get'><label>Zielplatz</label><select name='slot' required>";
      html += "<option value=''" + String(target < 0 ? " selected" : "") + " disabled>Bitte freien Platz wählen</option>";
      for (int slot = 0; slot < MAX_STOPS; ++slot) if (!stops[slot].length())
        html += "<option value='" + String(slot) + "'" + (slot == target ? " selected" : "") + ">Seite " + String(slot / STOPS_PER_PAGE + 1) + ", Platz " + String(slot % STOPS_PER_PAGE + 1) + "</option>";
      html += "</select><label for='q'>Name der Haltestelle</label><input id='q' name='q' type='text' minlength='2' maxlength='100' placeholder='z. B. Bahnhof' required><button class='full'>Haltestelle suchen</button></form>";
    }
    html += "</section><p class='hint'>Die Einstellungen gelten für das oben angezeigte Board. Unter „Meine Boards“ wechselst du zu einem anderen Display.</p>";
    sendPage(html);
  });
  server.on("/city", HTTP_POST, []() {
    String value = server.arg("city");
    if (!value.length() || value.length() > 2) { showMessage("Stadt fehlt."); return; }
    for (size_t i = 0; i < value.length(); ++i)
      if (value[i] < '0' || value[i] > '9') { showMessage("Stadt ungueltig."); return; }
    int index = value.toInt();
    if (index < 0 || index >= CITY_COUNT) { showMessage("Stadt ungueltig."); return; }
    changeCity(index);
    redirectHome();
  });
  server.on("/search", HTTP_GET, []() {
    String q = server.arg("q"); q.trim();
    int target = firstFreeSlotOnPage(activePage);
    if (server.hasArg("slot")) {
      String value = server.arg("slot");
      if (value.length() != 1 || value[0] < '0' || value[0] > '4') { showMessage("Bitte einen Zielplatz wählen."); return; }
      target = value.toInt();
    }
    if (target < 0 || stops[target].length()) { showMessage("Zielplatz belegt. Bitte einen freien Platz wählen."); return; }
    String html = pageStart("Suchergebnisse");
    html += "<section class='card'><h2>" + escapeHtml(q) + "</h2><ul>";
    if (q.length() >= 2 && q.length() <= 100 && WiFi.status() == WL_CONNECTED) {
      JsonDocument doc;
      if (getJson("https://transport.opendata.ch/v1/locations?type=station&query=" + urlEncode(citySearchQuery(q)) + "&fields%5B%5D=stations/name", doc)) {
        int n = 0;
        for (JsonObject row : doc["stations"].as<JsonArray>()) {
          String name = row["name"] | "";
          if (!name.length()) continue;
          if (n++ >= 20) break;
          bool selected = false;
          for (int i = 0; i < MAX_STOPS; ++i) if (stops[i] == name) selected = true;
          html += "<li><span>" + escapeHtml(name) + "</span>";
          if (selected) html += "<small>Ausgewählt</small>";
          else if (stopCount < MAX_STOPS) {
            html += "<form action='/add' method='post'><input type='hidden' name='name' value='" + escapeHtml(name) + "'><label>Zielplatz</label><select name='slot'>";
            for (int slot = 0; slot < MAX_STOPS; ++slot) if (!stops[slot].length())
              html += "<option value='" + String(slot) + "'" + (slot == target ? " selected" : "") + ">Seite " + String(slot / STOPS_PER_PAGE + 1) + ", Platz " + String(slot % STOPS_PER_PAGE + 1) + "</option>";
            html += "</select><button>Hinzufügen</button></form>";
          }
          else html += "<small>5 Plätze belegt</small>";
          html += "</li>";
        }
        if (!n) html += "<li>Keine Haltestelle gefunden. Versuche einen anderen Namen.</li>";
      } else html += "<li>Suche fehlgeschlagen. Bitte erneut versuchen.</li>";
    } else html += "<li>Mindestens zwei Zeichen und eine WLAN-Verbindung mit Internet sind erforderlich.</li>";
    html += "</ul><p><a class='button subtle' href='/'>Zurück zu meinen Haltestellen</a></p></section>";
    sendPage(html);
  });
  server.on("/add", HTTP_POST, []() {
    String name = server.arg("name"); name.trim();
    if (!name.length() || name.length() >= 100) { showMessage("Ungültiger Haltestellenname."); return; }
    for (int i = 0; i < MAX_STOPS; ++i) if (stops[i] == name) { redirectHome(); return; }
    if (stopCount >= MAX_STOPS) { showMessage("Es sind bereits fünf Haltestellen ausgewählt."); return; }
    String position = server.arg("slot");
    if (position.length() != 1 || position[0] < '0' || position[0] > '4') { showMessage("Bitte einen Platz auswaehlen."); return; }
    int slot = position.toInt();
    if (stops[slot].length()) { showMessage("Dieser Platz ist inzwischen belegt."); return; }
    stops[slot] = name; saveStops(); selectPage(slot / STOPS_PER_PAGE); lastRefresh = 0; drawBoard(); redirectHome();
  });
  // Web-App: aktive Anzeigeseite vom Handy aus wechseln.
  server.on("/page", HTTP_POST, []() {
    int page;
    if (!server.hasArg("page")) { showMessage("Seite fehlt."); return; }
    String value = server.arg("page");
    if (value.length() != 1 || value[0] < '0' || value[0] > '2') { showMessage("Ungueltige Seite."); return; }
    page = value.toInt();
    selectPage(page);
    lastRefresh = 0;
    drawBoard();
    redirectHome();
  });
  // Web-App: zwei Positionen tauschen; Reihenfolge dauerhaft speichern.
  server.on("/move", HTTP_POST, []() {
    if (!server.hasArg("slot")) { showMessage("Zielposition fehlt."); return; }
    int index, slot;
    if (!readIndex(MAX_STOPS, index)) { showMessage("Ungueltige Haltestelle."); return; }
    String value = server.arg("slot");
    if (value.length() != 1 || value[0] < '0' || value[0] > '4') { showMessage("Ungueltige Position."); return; }
    slot = value.toInt();

    if (slot != index) {
      String temp = stops[index];
      stops[index] = stops[slot];
      stops[slot] = temp;
      saveStops();
      selectPage(slot / STOPS_PER_PAGE);
      lastRefresh = 0;
      drawBoard();
    }
    redirectHome();
  });
  server.on("/remove", HTTP_POST, []() {
    int index;
    if (!readIndex(MAX_STOPS, index)) { showMessage("Ungültige Auswahl. Bitte Seite neu öffnen."); return; }
    removeStopAt(index); drawBoard(); redirectHome();
  });
  server.on("/boards", HTTP_GET, []() {
    String html = pageStart("Meine Boards");
    html += "<section class='card'><h2>" + escapeHtml(boardLabel()) + "</h2><p class='status'>Dieses Board · " + escapeHtml(currentAddress()) + "</p><a class='button full' href='/'>Dieses Board bedienen</a></section>";
    int count = prefs.getUChar("peerCount", 0);
    if (count > 12) count = 0;
    for (int i = 0; i < count; ++i) {
      String name = prefs.getString(("peerN" + String(i)).c_str(), "Board");
      String address = normalizeBoardAddress(prefs.getString(("peerA" + String(i)).c_str(), ""));
      html += "<section class='card'><h2>" + escapeHtml(name) + "</h2><p class='address'>" + escapeHtml(address) + "</p><p class='hint'>Erreichbarkeit nicht geprüft</p>";
      if (address.length()) html += "<a class='button full' href='" + escapeHtml(address) + "'>Board bedienen →</a>";
      html += "<form action='/boards/remove' method='post'><input type='hidden' name='index' value='" + String(i) + "'><p><button class='subtle'>Aus der Liste entfernen</button></p></form></section>";
    }
    html += "<section class='card'><h2>Weiteres Board hinzufügen</h2>";
    if (count < 12) html += "<form action='/boards/add' method='post'><label for='peerName'>Name</label><input id='peerName' name='name' maxlength='60' placeholder='z. B. Küche' required><label for='peerAddress'>IP-Adresse</label><input id='peerAddress' name='address' maxlength='100' placeholder='z. B. 10.128.0.36' autocapitalize='none' spellcheck='false' required><button class='full'>Board speichern</button></form>";
    else html += "<p>Die Liste enthält bereits 12 weitere Boards.</p>";
    html += "<p class='hint'>Die Liste wird auf diesem Board gespeichert. Sie wird nicht automatisch auf andere Boards übertragen. Alle Boards müssen von deinem Handy aus erreichbar sein. Über Safaris Zurück-Pfeil kommst du nach einem Wechsel zurück.</p></section>";
    sendPage(html);
  });
  server.on("/boards/add", HTTP_POST, []() {
    String name = server.arg("name"); name.trim();
    String address = normalizeBoardAddress(server.arg("address"));
    if (!name.length() || name.length() > 60 || !address.length()) { showMessage("Bitte Name und gültige IP-Adresse oder lokalen Hostnamen eingeben, ohne Port oder Unterseite."); return; }
    int count = prefs.getUChar("peerCount", 0);
    if (count > 12) count = 0;
    for (int i = 0; i < count; ++i) {
      if (prefs.getString(("peerA" + String(i)).c_str(), "") == address) {
        prefs.putString(("peerN" + String(i)).c_str(), name); redirectBoards(); return;
      }
    }
    if (count >= 12) { showMessage("Maximal 12 weitere Boards möglich."); return; }
    prefs.putString(("peerN" + String(count)).c_str(), name);
    prefs.putString(("peerA" + String(count)).c_str(), address);
    prefs.putUChar("peerCount", count + 1); redirectBoards();
  });
  server.on("/boards/remove", HTTP_POST, []() {
    int count = prefs.getUChar("peerCount", 0), index;
    if (count > 12 || !readIndex(count, index)) { showMessage("Ungültiges Board."); return; }
    for (int i = index; i < count - 1; ++i) {
      prefs.putString(("peerN" + String(i)).c_str(), prefs.getString(("peerN" + String(i + 1)).c_str(), ""));
      prefs.putString(("peerA" + String(i)).c_str(), prefs.getString(("peerA" + String(i + 1)).c_str(), ""));
    }
    prefs.remove(("peerN" + String(count - 1)).c_str());
    prefs.remove(("peerA" + String(count - 1)).c_str());
    prefs.putUChar("peerCount", count - 1); redirectBoards();
  });
  server.on("/settings", HTTP_GET, []() {
    String html = pageStart("Einstellungen");
    html += "<section class='card'><h2>Name dieses Boards</h2><form action='/device' method='post'><label for='boardname'>Name</label><input id='boardname' name='name' maxlength='60' value='" + escapeHtml(boardLabel()) + "' required><button class='full'>Namen speichern</button></form></section>";
    html += "<section class='card'><h2>Ulozene Wi-Fi site</h2>";
    html += "<p class='hint'>Az ctyri site, automaticke pripojeni a opakovani po vypadku.</p>";
    for(uint8_t i=0;i<profileCount;i++){
      html += "<p><b>" + escapeHtml(profileSsid[i]) + "</b>";
      if(WiFi.status()==WL_CONNECTED && WiFi.SSID()==profileSsid[i]) html += " (pripojeno)";
      html += "</p>";
      if(profileCount>1){
        html += "<form method='post' action='/wifi/remove'><input type='hidden' name='index' value='"+String(i)+"'>";
        html += "<label><input style='display:inline;width:auto;min-height:0' type='checkbox' name='confirm' value='yes' required> Potvrzuji odstraneni</label><button class='subtle'>Odebrat sit</button></form>";
      }
    }
    html += "<p><a class='button subtle' href='/wifi/scan'>Vyhledat dostupne Wi-Fi</a></p>";
    html += "<h2>Pridat nebo upravit Wi-Fi</h2>";
    html += "<form method='post' action='/wifi'><label>Nazev site SSID</label><input name='ssid' maxlength='32' required>";
    html += "<label>Heslo</label><input id='wpass' type='password' name='pass' maxlength='63' autocomplete='new-password'>";
    html += "<label><input style='display:inline;width:auto;min-height:0' type='checkbox' onclick=\"document.getElementById('wpass').type=this.checked?'text':'password'\"> Zobrazit heslo</label>";
    html += "<p class='hint'>Prazdne heslo u jiz ulozene site znamena zachovat puvodni heslo.</p>";
    html += "<button>Ulozit Wi-Fi</button></form></section>";
    html += "<p><a class='button' href='/diagnose'>Diagnostika odjezdu</a></p>";
    html += "<section class='card'><h2>Displej a firmware</h2>";
    html += "<p><a class='button' href='/display'>Zhasnuti a probuzeni</a></p>";
    html += "<p><a class='button' href='/ota'>Aktualizace firmware</a></p></section>";
    sendPage(html);
  });
  server.on("/device", HTTP_POST, []() {
    String name = server.arg("name"); name.trim();
    if (!name.length() || name.length() > 60) { showMessage("Bitte einen Namen mit höchstens 60 Zeichen eingeben."); return; }
    prefs.putString("boardname", name); redirectBoards();
  });
  server.on("/wifi/scan", HTTP_GET, []() {
    String html=pageStart("Vyhledat Wi-Fi");
    html+="<section class='card'><h2>Dostupne site</h2>";
    int state=WiFi.scanComplete();
    if(state==WIFI_SCAN_RUNNING) html+="<p>Hledani probiha, zkus obnovit za chvili.</p>";
    else if(state<0) {
      if(!otaAllowed()){WiFi.scanNetworks(true);html+="<p>Hledani spusteno, obnov stranku.</p>";}
      else html+="<p>Behem OTA skenovani vypnuto.</p>";
    } else {
      for(int i=0;i<state&&i<20;i++)html+="<p>"+escapeHtml(WiFi.SSID(i))+" ("+String(WiFi.RSSI(i))+" dBm)</p>";
      if(!state)html+="<p>Zadne site nebyly nalezeny.</p>";
      WiFi.scanDelete();
    }
    html+="<p><a class='button' href='/wifi/scan'>Obnovit vyhledavani</a></p>";
    html+="<p><a class='button subtle' href='/settings'>Zpet do nastaveni</a></p></section>";
    sendPage(html);
  });
  server.on("/wifi", HTTP_POST, []() {
    String ssid=server.arg("ssid"),pass=server.arg("pass");
    bool known=false;
    for(uint8_t i=0;i<profileCount;i++)if(profileSsid[i]==ssid){known=true;break;}
    if(!saveWiFiProfile(ssid,pass,pass.length() || !known)){
      showMessage("Neplatna Wi-Fi nebo limit 4 siti.");return;
    }
    wifiAttemptPending=false;
    wifiSwitchAt=millis()+2000;
    server.sendHeader("Location","/settings");server.send(303);
    // Nove ulozene heslo otestujeme pri nasledujicim automatickem pokusu. 
  });
  server.on("/wifi/remove", HTTP_POST, []() {
    if(server.arg("confirm")!="yes"){
      showMessage("Potvrd odstraneni site.");return;
    }
    String input=server.arg("index");
    if(input.length()!=1||input[0]<'0'||input[0]>'3'||
       !deleteWiFiProfile((uint8_t)input.toInt())) {
      showMessage("Neplatny vyber nebo posledni ulozena sit.");return;
    }
    server.sendHeader("Location","/settings");server.send(303);
  });
  server.on("/system", HTTP_GET, []() {
    String html=pageStart("System");
    html+="<section class='card'><h2>System</h2>";
    html+="<p>Firmware: <b>" TRAMLIBERN_VERSION "</b></p>";
    html+="<p>OTA partition: "+String((unsigned)otaMaxBytes())+" B</p>";
    html+="<p>Volna RAM: "+String(ESP.getFreeHeap())+" B</p>";
    html+="<p><a class='button' href='/display'>Displej, jas a baterie</a></p>";
    html+="<p><a class='button' href='/diagnose'>Diagnostika</a></p>";
    html+="<p><a class='button' href='/ota'>OTA firmware</a></p>";
    html+="<form action='/refresh' method='post'><button>Aktualizovat odjezdy</button></form></section>";
    sendPage(html);
  });
  server.on("/display", HTTP_GET, []() {
    String html = pageStart("Displej");
    html += "<section class='card'><h2>Displej</h2><p>Stav: " +
            String(displaySleeping ? "Zhasnuty" : "Zapnuty") + "</p>";
    html += "<form method='post' action='/display/wake'><button class='full'>Probudit displej</button></form>";
    html += "<form method='post' action='/display/off'><button class='full subtle'>Zhasnout displej</button></form>";
    html += "<form method='post' action='/display/config'><label><input style='display:inline;width:auto;min-height:0' type='checkbox' name='auto' value='1'";
    if (displayAutoOff) html += " checked";
    html += "> Automaticky zhasinat</label><label>Po minutach (1 az 240)</label><input name='minutes' type='number' min='1' max='240' value='" + String(displayIdleMinutes) + "'>";
    html += "<button class='full'>Ulozit nastaveni</button></form>";
    html += "<h2>Jas a nocni rezim (PWM experimentalni)</h2>";
    html += "<p class='notice'>GPIO27 je overen jen pro zapnuti/vypnuti. PWM stmivani nejprve otestuj na fyzickem displeji.</p>";
    html += "<form method='post' action='/display/brightness'>";
    html += "<label><input style='display:inline;width:auto;min-height:0' type='checkbox' name='enable' value='1'";
    if(brightnessPwmEnabled)html+=" checked";
    html += "> Zapnout experimentalni PWM</label>";
    html += "<label>Denni jas 35-100%</label><input type='number' min='35' max='100' name='day' value='"+String(dayBrightness)+"'>";
    html += "<label>Nocni jas 15-100%</label><input type='number' min='15' max='100' name='night' value='"+String(nightBrightness)+"'>";
    html += "<label><input style='display:inline;width:auto;min-height:0' type='checkbox' name='schedule' value='1'";
    if(nightEnabled)html+=" checked";
    html += "> Nocni rezim podle casu</label>";
    html += "<label>Noc od (0-23)</label><input type='number' min='0' max='23' name='from' value='"+String(nightFrom)+"'>";
    html += "<label>Den od (0-23)</label><input type='number' min='0' max='23' name='to' value='"+String(nightTo)+"'>";
    html += "<button>Ulozit jas</button></form><h2>Baterie</h2>";
    html += "<p>Orientacni napeti: "+String(batteryVoltage(),2)+" V</p>";
    html += "<form method='post' action='/display/battery'>";
    html += "<label><input style='display:inline;width:auto;min-height:0' type='checkbox' name='enable' value='1'";
    if(batteryWarningEnabled)html+=" checked";
    html += "> Zapnout varovani</label>";
    html += "<label>Prag 3000-3850 mV</label><input type='number' min='3000' max='3850' name='mv' value='"+String(batteryThresholdMv)+"'>";
    html += "<button>Ulozit baterii</button></form>";
    html += "<p class='hint'>Po zapnuti napajeni je displej vzdy rozsviceny. Prvni dotyk po zhasnuti pouze probudi.</p></section>";
    sendPage(html);
  });
  server.on("/display/brightness", HTTP_POST, []() {
    int day=server.arg("day").toInt(),night=server.arg("night").toInt();
    int from=server.arg("from").toInt(),to=server.arg("to").toInt();
    if(day<35||day>100||night<15||night>100||from<0||from>23||to<0||to>23){showMessage("Neplatny jas nebo cas.");return;}
    dayBrightness=day;nightBrightness=night;nightFrom=from;nightTo=to;
    brightnessPwmEnabled=server.arg("enable")=="1";
    nightEnabled=server.arg("schedule")=="1";
    prefs.putUChar("dayBri",dayBrightness);prefs.putUChar("nightBri",nightBrightness);
    prefs.putUChar("nightFrom",nightFrom);prefs.putUChar("nightTo",nightTo);
    prefs.putBool("pwmOptIn",brightnessPwmEnabled);prefs.putBool("nightOn",nightEnabled);
    applyBacklight();server.sendHeader("Location","/display");server.send(303);
  });
  server.on("/display/battery", HTTP_POST, []() {
    int mv=server.arg("mv").toInt();
    if(mv<3000||mv>3850){showMessage("Neplatny prag baterie.");return;}
    batteryWarningEnabled=server.arg("enable")=="1";
    batteryThresholdMv=mv;
    prefs.putBool("batWarnOn",batteryWarningEnabled);prefs.putUShort("batWarnMv",mv);
    ++screenRevision;server.sendHeader("Location","/display");server.send(303);
  });
  server.on("/display/off", HTTP_POST, []() {
    displaySleep();
    server.sendHeader("Location", "/display"); server.send(303);
  });
  server.on("/display/wake", HTTP_POST, []() {
    displayWake();
    server.sendHeader("Location", "/display"); server.send(303);
  });
  server.on("/display/config", HTTP_POST, []() {
    int n=server.arg("minutes").toInt();
    if (n < 1 || n > 240) { showMessage("Vyber 1 az 240 minut."); return; }
    displayIdleMinutes = n;
    displayAutoOff = server.hasArg("auto") && server.arg("auto") == "1";
    prefs.putBool("dispAuto", displayAutoOff);
    prefs.putUShort("dispMin", displayIdleMinutes);
    displayLastActivity = millis();
    server.sendHeader("Location", "/display"); server.send(303);
  });
  server.on("/ota", HTTP_GET, []() {
    String html=pageStart("Aktualizace firmware");
    html += "<section class='card'><h2>Aktualizace pres Safari</h2>";
    html += "<p>Pro nahrani nejdriv na displeji stiskni MENU a OTA 5 MIN.</p>";
    html += "<p>Povoleni: " + String(otaAllowed()?"AKTIVNI":"NEAKTIVNI") + "</p>";
    html += "<p>Maximalni velikost firmware pro nahrani: " + String((unsigned)otaMaxBytes()) + " B.</p>";
    if (otaAllowed()) {
      uint32_t remaining = (otaAllowedUntil - millis()) / 1000UL;
      html += "<p>Cas zbyvajici pro zahajeni nahravani: " + String(remaining) + " s</p>";
    }
    html += "<p class='hint'>Pouze aplikacni ESP32 obraz (.ino.bin). Pred prenosem porovnej nazev zarizeni a velikost souboru. Zmena partition table pri nahrani OTA neni mozna.</p>";
    html += "<script>const otaNonce='" + String(otaAllowed() ? otaSessionNonce : 0, HEX) + "';</script>";
    html += "<p class='notice'>Nahravej vyhradne samostatny .ino.bin urceny pro tuto desku, ne merged.bin!</p>";
    html += R"HTML(<input type='file' id='fw' accept='.bin'><button id='send' onclick='go()'>Nahrat firmware</button><p id='status'></p>
<script>
async function go(){const f=document.getElementById('fw').files[0],status=document.getElementById('status');
 if(!f || !f.name.endsWith('.ino.bin')){status.textContent='Vyber samostatny soubor .ino.bin';return;}
 status.textContent='Overuji velikost...';
 try {const test=await fetch('/ota/check?size='+f.size+'&nonce='+otaNonce,{cache:'no-store'});
 if(!test.ok){status.textContent=await test.text();return;}
 const form=new FormData();form.append('firmware',f,f.name);
 const xhr=new XMLHttpRequest();xhr.open('POST','/ota/upload?size='+f.size+'&nonce='+otaNonce);
 xhr.upload.onprogress=e=>{if(e.lengthComputable)status.textContent='Prenos do ESP32: '+Math.floor(e.loaded/e.total*100)+' % (po dokonceni probiha kontrola)';};
 xhr.onload=()=>{status.textContent=xhr.responseText;};
 xhr.onerror=()=>{status.textContent='Nahravani selhalo. Zkus znovu.';};xhr.send(form);
 }catch(err){status.textContent='Chyba spojeni: '+err;}}
</script>)HTML";
    html += "</section>";
    sendPage(html);
  });
  server.on("/ota/check", HTTP_GET, []() {
    if (!otaSessionAuthorized()) {
      server.send(403,"text/plain; charset=utf-8","Povol OTA 5 MIN na displeji a obnov stranku v Safari."); return;
    }
    size_t expected = 0;
    if (!otaParseFirmwareSize(server.arg("size"), expected)) {
      server.send(413,"text/plain; charset=utf-8","Soubor ma neplatnou velikost nebo se nevejde do OTA oblasti."); return;
    }
    server.send(200,"text/plain; charset=utf-8","OK");
  });
  server.on("/ota/upload", HTTP_POST, []() {
    bool ok=otaFinishedOK && otaUploadOK && otaUploadStarted &&
            otaUploadedBytes == otaExpectedBytes && !Update.hasError();
    String response=ok ? "Firmware overen a ulozen. ESP32 se restartuje." :
                         "Aktualizace selhala: " + (otaUploadError.length() ? otaUploadError : "Neznamy problem");
    server.sendHeader("Cache-Control", "no-store");
    server.send(ok ? 200 : 400, "text/plain; charset=utf-8", response);
    otaFinishedOK=false;
    otaUploadStarted=false;
    otaUploadOK=false;
    if (ok) {otaAllowedUntil=0;otaSessionNonce=0;delay(500);ESP.restart();}
  }, []() {
    HTTPUpload &upload=server.upload();
    if(upload.status==UPLOAD_FILE_START) {
      otaUploadStarted=false;otaUploadOK=false;otaFinishedOK=false;
      otaUploadedBytes=0;otaExpectedBytes=0;
      otaUploadError="OTA neni povoleno nebo je soubor neplatny";
      if (!otaSessionAuthorized()) return;
      if (!upload.filename.endsWith(".ino.bin")) {
        otaUploadError="Vyber samostatny .ino.bin, ne merged.bin";return;
      }
      size_t expected=0;
      if (!otaParseFirmwareSize(server.arg("size"), expected)) {
        otaUploadError="Soubor se nevejde do aktivni OTA oblasti";return;
      }
      if (!Update.begin(expected, U_FLASH)) {
        otaUploadError="Nelze zahajit zapis do OTA oblasti";return;
      }
      otaExpectedBytes=expected;
      otaUploadStarted=true;otaUploadOK=true;otaUploadError="";
      // Only one upload is accepted during a physical 5-minute authorization.
      otaAllowedUntil=0;otaSessionNonce=0;
    } else if(upload.status==UPLOAD_FILE_WRITE) {
      if (!otaUploadOK || !otaUploadStarted) return;
      if (!otaUploadedBytes && upload.currentSize && upload.buf[0] != 0xE9) {
        otaUploadError="Soubor neni platny obraz firmware ESP32";
        otaUploadOK=false;Update.abort();return;
      }
      if (upload.currentSize > otaExpectedBytes-otaUploadedBytes) {
        otaUploadError="Bylo prijato vice dat nez velikost souboru";
        otaUploadOK=false;Update.abort();return;
      }
      if (Update.write(upload.buf,upload.currentSize) != upload.currentSize) {
        otaUploadError="Chyba zapisu OTA firmware";
        otaUploadOK=false;Update.abort();return;
      }
      otaUploadedBytes+=upload.currentSize;
    } else if (upload.status==UPLOAD_FILE_END) {
      if (!otaUploadOK || !otaUploadStarted) return;
      if (!otaExpectedBytes || otaUploadedBytes != otaExpectedBytes) {
        otaUploadError="Prenos nebyl kompletni (chybi cast firmware)";
        otaUploadOK=false;Update.abort();return;
      }
      // Strict end: do not accept an incomplete image or pad missing bytes.
      if (!Update.end(false)) {
        otaUploadError="Overeni firmware ESP32 selhalo";
        otaUploadOK=false;return;
      }
      otaFinishedOK=true;
    } else if(upload.status==UPLOAD_FILE_ABORTED) {
      if (otaUploadStarted && !otaFinishedOK) Update.abort();
      otaFinishedOK=false;otaUploadOK=false;
      otaUploadError="Prenos prerusen. Puvodni firmware nebyl prepsan.";
    }
  });
  server.on("/refresh", HTTP_POST, []() { lastRefresh = 0; redirectHome(); });
  server.on("/refresh", HTTP_GET, []() { lastRefresh = 0; redirectHome(); });
  // Neuer optionaler Kalibrierungs-Link; bestehende Web-App bleibt unveraendert.
  server.on("/touch-kalibrieren", HTTP_GET, []() {
    server.send(200, "text/plain; charset=utf-8", "Kalibrierung wird gestartet. Bitte die drei Kreuze am Display antippen.");
    Preferences tp; tp.begin("tramli-touch", false); tp.putBool("valid", false); tp.end();
    delay(100); ESP.restart();
  });
  server.onNotFound([]() { showMessage("Diese Seite existiert nicht.", 404); });
  server.begin();
}
float batteryVoltage() {
  return analogReadMilliVolts(34) * 2.0 / 1000.0;
}
// Touch-Erweiterung 2.7: die bestehende Web-App bleibt unveraendert.
// XPT2046-Rohwerte: Standard-Kalibrierung. Bei verschobenen Beruehrungen
// die vier Grenzen mit dem seriellen Monitor an das eigene Display anpassen.
// Display-Kalibrierung: drei sichtbare Punkte; beruecksichtigt auch vertauschte
// oder gespiegelte X/Y-Achsen. Wird im NVS separat gespeichert.
int32_t calRawX[3] = {0, 0, 0}, calRawY[3] = {0, 0, 0};
bool calValid = false;
int touchXFromRaw(int rawX, int rawY) {
  if (!calValid) return -1;
  const double a = calRawX[1] - calRawX[0], b = calRawX[2] - calRawX[0];
  const double c = calRawY[1] - calRawY[0], d = calRawY[2] - calRawY[0];
  const double det = a*d - b*c;
  if (det > -10000.0 && det < 10000.0) return -1;
  const double u = ((rawX-calRawX[0])*d - (rawY-calRawY[0])*b)/det;
  return constrain((int)(60.0 + 360.0*u + 0.5), 0, 479);
}
int touchYFromRaw(int rawX, int rawY) {
  if (!calValid) return -1;
  const double a = calRawX[1] - calRawX[0], b = calRawX[2] - calRawX[0];
  const double c = calRawY[1] - calRawY[0], d = calRawY[2] - calRawY[0];
  const double det = a*d - b*c;
  if (det > -10000.0 && det < 10000.0) return -1;
  const double v = (a*(rawY-calRawY[0]) - c*(rawX-calRawX[0]))/det;
  return constrain((int)(60.0 + 200.0*v + 0.5), 0, 319);
}
void loadTouchCalibration() {
  Preferences tp; tp.begin("tramli-touch", true);
  calValid = tp.getBool("valid", false);
  for (int i=0; i<3; ++i) {
    calRawX[i] = tp.getInt(("x"+String(i)).c_str(), 0);
    calRawY[i] = tp.getInt(("y"+String(i)).c_str(), 0);
  }
  tp.end();
}
void calibrateTouch() {
  if (!touchReady || calValid) return;
  const int tx[3]={60,420,60}, ty[3]={60,60,260};
  gfx->fillScreen(TRAM_COLOR_BLACK);
  drawLine("TOUCH KALIBRIEREN", 100, 8, TRAM_COLOR_AMBER, 2);
  drawLine("3 Kreuze nacheinander tippen", 105, 30, TRAM_COLOR_WHITE, 1);
  for(int i=0; i<3; ++i) {
    gfx->fillRect(0,48,480,272,TRAM_COLOR_BLACK);
    drawLine("Punkt " + String(i+1) + " von 3", 180, 152, TRAM_COLOR_WHITE, 2);
    gfx->drawCircle(tx[i], ty[i], 13, TRAM_COLOR_AMBER);
    gfx->drawFastHLine(tx[i]-18,ty[i],37, TRAM_COLOR_AMBER);
    gfx->drawFastVLine(tx[i],ty[i]-18,37, TRAM_COLOR_AMBER);
    // Punkt 3 darf unten liegen; Infotext ueberdeckt das Kreuz nicht.
    unsigned long started=millis();
    bool got=false;
    while(millis()-started<90000UL) {
      if(touch.touched()) {
        delay(50);
        TS_Point p=touch.getPoint();
        if(p.z>80) {
          calRawX[i]=p.x; calRawY[i]=p.y;
          Serial.printf("Kalibrierung %d: %ld %ld\n",i+1,(long)p.x,(long)p.y);
          while(touch.touched()) delay(15);
          delay(180); got=true; break;
        }
      }
      delay(15);
    }
    if(!got) {
      gfx->fillScreen(TRAM_COLOR_BLACK);
      drawLine("Kalibrierung abgebrochen", 12, 80, TRAM_COLOR_AMBER, 2);
      delay(1200);
      return;
    }
  }
  const double a=calRawX[1]-calRawX[0], b=calRawX[2]-calRawX[0];
  const double c=calRawY[1]-calRawY[0], d=calRawY[2]-calRawY[0];
  const double det=a*d-b*c;
  if(det>-10000.0 && det<10000.0) {
    gfx->fillScreen(TRAM_COLOR_BLACK);
    drawLine("Kalibrierung ungueltig", 12, 80, TRAM_COLOR_AMBER, 2);
    delay(1500); return;
  }
  Preferences tp; tp.begin("tramli-touch", false);
  for(int i=0; i<3; ++i) {
    tp.putInt(("x"+String(i)).c_str(),calRawX[i]);
    tp.putInt(("y"+String(i)).c_str(),calRawY[i]);
  }
  tp.putBool("valid",true); tp.end();
  calValid=true;
  gfx->fillScreen(TRAM_COLOR_BLACK);
  drawLine("Kalibrierung gespeichert", 60, 120, TRAM_COLOR_AMBER, 2);
  delay(1200);
}


void touchButton(const String &label, int x, int y, int w, int h, uint16_t color = TRAM_COLOR_AMBER) {
  gfx->drawRoundRect(x, y, w, h, 4, color);
  // Textgroesse 2 = 12 Pixel pro Zeichen.
  int textX = x + max(3, (w - (int)label.length() * 12) / 2);
  drawLine(label, textX, y + (h - 16) / 2, color, 2);
}

// Two columns, seven rows. Gaps and the unused final cell are not selectable.
int cityAtTouch(int x, int y) {
  if (x < 12 || x >= 468 || y < 43 || y >= 302) return -1;
  int column = (x - 12) / 234;
  int row = (y - 43) / 37;
  if ((x - 12) % 234 >= 222 || (y - 43) % 37 >= 33) return -1;
  int city = row * 2 + column;
  return city < CITY_COUNT ? city : -1;
}
void drawCityMenu() {
  gfx->fillScreen(TRAM_COLOR_BLACK);
  drawLine("STADT AUSWAEHLEN", 12, 6, TRAM_COLOR_AMBER, 2);
  touchButton("ABBRUCH", 352, 4, 116, 25, TRAM_COLOR_WHITE);
  gfx->drawFastHLine(10, 33, 460, TRAM_COLOR_AMBER);
  for (int city = 0; city < CITY_COUNT; ++city) {
    int x = 12 + (city % 2) * 234;
    int y = 43 + (city / 2) * 37;
    if (city == activeCity) gfx->fillRoundRect(x, y, 222, 33, 4, BUTTON_BLUE);
    touchButton(CITY_LABELS[city], x, y, 222, 33, TRAM_COLOR_WHITE);
  }
}
void drawMainMenu() {
  gfx->fillScreen(TRAM_COLOR_BLACK);
  drawLine("TRAMLIBERN MENU",12,7,TRAM_COLOR_AMBER,2);
  touchButton("ZURUECK",350,3,118,28,TRAM_COLOR_WHITE);
  gfx->drawFastHLine(10,35,460,TRAM_COLOR_AMBER);
  touchButton("STADT",30,41,420,35);
  touchButton("HALTESTELLEN",30,82,420,35);
  touchButton("SEITEN 1 / 2 / 3",30,123,420,35);
  touchButton("WLAN",30,164,420,35);
  touchButton("DIAGNOSE",30,205,420,35);
  touchButton("SYSTEM",30,246,420,35);
}
void drawPageSelectMenu() {
  gfx->fillScreen(TRAM_COLOR_BLACK);
  drawLine("SEITEN EINRICHTEN",12,7,TRAM_COLOR_AMBER,2);
  touchButton("ZURUECK",350,3,118,28,TRAM_COLOR_WHITE);
  gfx->drawFastHLine(10,35,460,TRAM_COLOR_AMBER);
  for(int page=0;page<3;page++){
    touchButton("SEITE "+String(page+1)+(page==activePage?" AKTIV":""),25,52+64*page,430,50);
  }
}
void drawSystemMenu() {
  gfx->fillScreen(TRAM_COLOR_BLACK);
  drawLine("TRAMLIBERN SYSTEM",12,7,TRAM_COLOR_AMBER,2);
  touchButton("ZURUECK",350,3,118,28,TRAM_COLOR_WHITE);
  gfx->drawFastHLine(10,35,460,TRAM_COLOR_AMBER);
  touchButton("OTA 5 MIN",25,55,430,44);
  touchButton("DISPLAY AUS",25,113,430,44);
  drawLine("Usporne nastaveni a OTA v Safari",12,192,TRAM_COLOR_AMBER,1);
  drawLine("Safari: /display nebo /ota",12,220,TRAM_COLOR_WHITE,1);
}
void drawWifiMenu() {
  gfx->fillScreen(TRAM_COLOR_BLACK);
  drawLine("ULOZENE WLAN",12,7,TRAM_COLOR_AMBER,2);
  touchButton("ZURUECK",350,3,118,28,TRAM_COLOR_WHITE);
  gfx->drawFastHLine(10,35,460,TRAM_COLOR_AMBER);
  for(int i=0;i<profileCount;i++){
    String label=profileSsid[i];
    if(WiFi.status()==WL_CONNECTED && WiFi.SSID()==label)label+=" *";
    touchButton(clip(label,30),16,51+i*49,448,43);
  }
  if(profileCount==0)drawLine("Keine gespeicherte WLAN",20,95,TRAM_COLOR_WHITE,2);
  drawLine("Pridani a hesla: Safari /settings",12,276,TRAM_COLOR_WHITE,1);
}
void drawBoardDiagnostic() {
  gfx->fillScreen(TRAM_COLOR_BLACK);
  drawLine("ABFAHRTEN DIAGNOSE",12,7,TRAM_COLOR_AMBER,2);
  touchButton("ZURUECK",350,3,118,28,TRAM_COLOR_WHITE);
  drawLine(WiFi.status()==WL_CONNECTED?"WLAN: VERBUNDEN":"WLAN: GETRENNT",12,53,TRAM_COLOR_WHITE,2);
  drawLine(clip(statusText,34),12,82,TRAM_COLOR_AMBER,1);
  for(int pos=0;pos<(activePage==2?1:2);pos++){
    int i=activePage*2+pos;
    if(!stops[i].length())continue;
    int y=116+pos*77;
    drawLine(clip(stops[i],33),12,y,TRAM_COLOR_WHITE,1);
    if(lastSuccess[i])drawLine("Data pred "+String((millis()-lastSuccess[i])/60000UL)+" min",12,y+20,TRAM_COLOR_AMBER,1);
    else drawLine("Zatim bez dat",12,y+20,TRAM_COLOR_AMBER,1);
    if(stopFetchFailed[i])drawLine("CHYBA: detaily v Safari",12,y+39,TRAM_COLOR_WHITE,1);
  }
  drawLine("Podrobnosti: Safari /diagnose",12,281,TRAM_COLOR_WHITE,1);
}
void drawTouchMenu() {
  gfx->fillScreen(TRAM_COLOR_BLACK);
  drawLine("HALTESTELLE AENDERN", 12, 6, TRAM_COLOR_AMBER, 2);
  gfx->drawFastHLine(10, 30, 460, TRAM_COLOR_AMBER);
  if (touchEditIndex < 0 || touchEditIndex >= MAX_STOPS) return;
  String oldName = stops[touchEditIndex].length() ? stops[touchEditIndex] : "Freier Platz";
  drawLine(clip(oldName, 36), 12, 38, TRAM_COLOR_WHITE, 1);
  touchButton("ABBRUCH", 352, 4, 116, 25, TRAM_COLOR_WHITE);
  if (touchMode == 3) {
    drawLine("Haltestelle entfernen?", 12, 88, TRAM_COLOR_WHITE, 2);
    drawLine("Andere Plaetze bleiben erhalten.", 12, 120, TRAM_COLOR_WHITE, 1);
    touchButton("JA, ENTFERNEN", 12, 170, 220, 45);
    touchButton("ZURUECK", 248, 170, 220, 45, TRAM_COLOR_WHITE);
  } else if (touchMode == 1) {
    gfx->fillRect(10, 57, 460, 26, TRAM_COLOR_BLACK);
    gfx->drawRect(10, 57, 460, 26, TRAM_COLOR_AMBER);
    drawLine(clip(touchQuery.length() ? touchQuery : "Suchbegriff eingeben", 36), 16, 62, TRAM_COLOR_WHITE, 2);
    const char *rows[] = {"QWERTZUIOP", "ASDFGHJKL", "YXCVBNM"};
    for (int r = 0; r < 3; ++r) {
      const String line(rows[r]);
      int keyW = 44, start = r == 2 ? 84 : r == 1 ? 42 : 16;
      for (int c = 0; c < (int)line.length(); ++c) {
        touchButton(line.substring(c,c+1), start + c * keyW, 88 + r * 42, 40, 37);
      }
    }
    // Deutsche Sonderzeichen. Der Standardfont kann Umlaute nicht direkt zeichnen;
    // die Tasten zeigen AE/OE/UE/SS, suchen aber nach ae/oe/ue/ss als Umlaute.
    touchButton("AE", 68, 214, 78, 31);
    touchButton("OE", 156, 214, 78, 31);
    touchButton("UE", 244, 214, 78, 31);
    touchButton("SS", 332, 214, 78, 31);
    touchButton("LEER", 12, 251, 100, 35);
    touchButton("LOESCH", 126, 251, 120, 35);
    touchButton("SUCHE", 260, 251, 205, 35);
    if (stops[touchEditIndex].length()) touchButton("HALTESTELLE ENTFERNEN", 12, 290, 454, 28, TRAM_COLOR_WHITE);
  } else {
    drawLine("Suchresultate antippen:", 12, 65, TRAM_COLOR_WHITE, 2);
    if (!touchMatchCount) drawLine("Keine Treffer - erneut suchen", 12, 100, TRAM_COLOR_AMBER, 2);
    for (int i = 0; i < touchMatchCount; ++i) {
      int y = 91 + i * 37;
      touchButton(clip(touchMatches[i], 34), 12, y, 454, 34);
    }
    touchButton("ZURUECK", 12, 282, 454, 32, TRAM_COLOR_WHITE);
  }
}

void closeTouchMenu() {
  touchMenuOpen = false;
  touchMode = 0;
  touchEditIndex = -1;
  ++touchExitRevision;
  drawBoard();
}

void touchSearch() {
  if (WiFi.status() != WL_CONNECTED) { drawLine("WLAN erforderlich", 12, 294, TRAM_COLOR_WHITE, 1); return; }
  touchQuery.trim();
  if (touchQuery.length() < 2) { drawLine("Mindestens 2 Zeichen", 12, 294, TRAM_COLOR_WHITE, 1); return; }
  touchButton("SUCHE...", 260, 246, 205, 40, TRAM_COLOR_WHITE);
  JsonDocument doc;
  touchMatchCount = 0;
  bool ok = getJson("https://transport.opendata.ch/v1/locations?type=station&query=" +
    urlEncode(citySearchQuery(touchQuery)) + "&fields%5B%5D=stations/name", doc);
  if (ok) {
    for (JsonObject row : doc["stations"].as<JsonArray>()) {
      String name = row["name"] | "";
      if (!name.length()) continue;
      bool duplicate = false;
      for (int j=0; j<touchMatchCount; ++j) if (touchMatches[j] == name) duplicate=true;
      if (!duplicate) touchMatches[touchMatchCount++] = name;
      if (touchMatchCount >= 5) break;
    }
  }
  touchMode = 2;
  drawTouchMenu();
}

void handleTouchTap(int rawX, int rawY) {
  int x = touchXFromRaw(rawX, rawY), y = touchYFromRaw(rawX, rawY);
  if (x < 0 || y < 0) return;
  Serial.printf("Touch raw %d,%d -> px %d,%d | mode %d\n", rawX, rawY, x, y, touchMode);
  displayLastActivity = millis();
  if (!touchMenuOpen) {
    if (y >= 270 && y < 295 && x >= 230 && x < 312) {
      touchMenuOpen = true; touchMode = 5; drawMainMenu(); return;
    }
    // Direct page selection also works for empty pages.
    if (y >= 270 && y < 295 && x >= 320 && x < 470) {
      selectPage((x - 320) / 50);
      lastRefresh = 0;
      drawBoard();
      return;
    }
    // Mesto lze vybrat pouze pres MENU; stranky pres 1/2/3 nebo swipe.
    // Nur der Haltestellentitel oeffnet das Bearbeitungsmenue.
    if (x > 8 && x < 470) {
      int position = y >= 39 && y < 63 ? 0 : (activePage < 2 && y >= 149 && y < 172 ? 1 : -1);
      if (position >= 0) {
        touchEditIndex = activePage * STOPS_PER_PAGE + position;
        if (touchEditIndex >= MAX_STOPS) return;
        touchMenuOpen = true;
        touchMode = 1;
        touchQuery = "";
        drawTouchMenu();
      }
    }
    return;
  }
  if (y < 32 && x > 345) { closeTouchMenu(); return; }
  if (touchMode == 5) { // hlavni MENU
    if(x>=30&&x<450) {
      if(y>=41&&y<76){touchMode=4;drawCityMenu();return;}
      if(y>=82&&y<117){touchEditIndex=activePage*2;touchQuery="";touchMode=1;drawTouchMenu();return;}
      if(y>=123&&y<158){touchMode=9;drawPageSelectMenu();return;}
      if(y>=164&&y<199){touchMode=7;drawWifiMenu();return;}
      if(y>=205&&y<240){touchMode=6;drawBoardDiagnostic();return;}
      if(y>=246&&y<281){touchMode=8;drawSystemMenu();return;}
    }
    return;
  }
  if(touchMode==9) {
    if(x>=25&&x<455&&y>=52&&y<244){
      int page=(y-52)/64;
      if((y-52)%64<50&&page>=0&&page<PAGE_COUNT){
        selectPage(page);closeTouchMenu();lastRefresh=0;
      }
    }
    return;
  }
  if(touchMode==6){closeTouchMenu();return;}
  if(touchMode==7){ // kliknout na ulozenou sit
    if(x>=16&&x<464&&y>=51&&y<51+49*profileCount){
      int i=(y-51)/49;
      if((y-51)%49<43&&i<profileCount){
        profilePreferred=i;
        wifiAttemptPending=false; WiFi.disconnect();wifiNextTryAt=0;
        closeTouchMenu();
      }
    }
    return;
  }
  if(touchMode==8){
    if(x>=25&&x<455&&y>=55&&y<99){
      otaSessionNonce=esp_random();
      if (!otaSessionNonce) otaSessionNonce=1;
      otaAllowedUntil=millis()+300000UL;
      drawSystemMenu();drawLine("OTA AKTIV 5 MIN",12,294,TRAM_COLOR_AMBER,1);
    } else if(x>=25&&x<455&&y>=113&&y<157){
      closeTouchMenu();displaySleep();
    }
    return;
  }
  if (touchMode == 4) {
    int city = cityAtTouch(x, y);
    if (city >= 0) { changeCity(city); closeTouchMenu(); }
    return;
  }
  if (touchMode == 3) {
    if (y >= 170 && y < 215) {
      if (x >= 12 && x < 232) { removeStopAt(touchEditIndex); closeTouchMenu(); }
      else if (x >= 248 && x < 468) { touchMode = 1; drawTouchMenu(); }
    }
    return;
  }
  if (touchMode == 1 && y >= 290 && y < 318 && x >= 12 && x < 466 &&
      touchEditIndex >= 0 && touchEditIndex < MAX_STOPS && stops[touchEditIndex].length()) {
    touchMode = 3; drawTouchMenu(); return;
  }
  if (touchMode == 2) {
    if (y >= 282) { touchMode = 1; drawTouchMenu(); return; }
    if (y >= 91 && y < 91 + 5*37) {
      int result = (y - 91) / 37;
      if (result >= 0 && result < touchMatchCount) {
        String chosen = touchMatches[result];
        // Bereits verwendete Haltestellen nicht versehentlich doppelt speichern.
        for (int i=0; i<MAX_STOPS; ++i) {
          if (i != touchEditIndex && stops[i] == chosen) {
            touchMode = 1; drawTouchMenu();
            drawLine("Haltestelle bereits vorhanden", 12, 293, TRAM_COLOR_WHITE, 1);
            return;
          }
        }
        if (touchEditIndex < 0 || touchEditIndex >= MAX_STOPS) { closeTouchMenu(); return; }
        stops[touchEditIndex] = chosen;
        saveStops(); // Gleiches Preferences-System wie in der Web-App.
        lastRefresh = 0;
        closeTouchMenu();
      }
    }
    return;
  }
  if (y >= 214 && y < 245) {
    if (x >= 68 && x < 146) touchQuery += "ä";
    else if (x >= 156 && x < 234) touchQuery += "ö";
    else if (x >= 244 && x < 322) touchQuery += "ü";
    else if (x >= 332 && x < 410) touchQuery += "ß";
    if (touchQuery.length() > 70) touchQuery.remove(70);
    drawTouchMenu(); return;
  }
  if (y >= 251 && y < 286) {
    if (x < 113) { if (touchQuery.length() < 70) touchQuery += " "; }
    else if (x >= 126 && x < 246) {
      // Ein UTF-8-Zeichen komplett loeschen (auch ae/oe/ue/ss-Umlaute).
      if (touchQuery.length()) {
        int last = touchQuery.length() - 1;
        while (last > 0 && ((uint8_t)touchQuery[last] & 0xC0) == 0x80) --last;
        touchQuery.remove(last);
      }
    }
    else if (x >= 260) { touchSearch(); return; }
    drawTouchMenu(); return;
  }
  if (y >= 88 && y < 88 + 3*42) {
    int r = (y - 88) / 42;
    const char *rows[] = {"QWERTZUIOP", "ASDFGHJKL", "YXCVBNM"};
    int start = r == 2 ? 84 : r == 1 ? 42 : 16;
    int c = (x - start) / 44;
    if (x >= start && c >= 0 && c < (int)strlen(rows[r]) && x < start + c * 44 + 40) {
      if (touchQuery.length() < 70) touchQuery += rows[r][c];
      drawTouchMenu();
    }
  }
}

// Der XPT2046 teilt sich SPI mit dem Bildschirm. Nicht gleichzeitig zeichnen und lesen.
// setRotation(1) entspricht der Querformat-Ausrichtung der Anzeige.
void setupTouch() {
  pinMode(36, INPUT);
  touchSPI.begin(14, 12, 13, 33);
  touchReady = touch.begin(touchSPI);
  if (touchReady) touch.setRotation(1);
  Serial.println(touchReady ? "Touch bereit" : "Touch nicht gefunden");
}

void checkSwipe() {
  if (!touchReady || !calValid) return;
  if (displaySleeping) {
    if (touch.touched()) { displayWake(); wakeTouchWaitRelease = true; }
    return;
  }
  if (wakeTouchWaitRelease) {
    if (!touch.touched()) wakeTouchWaitRelease = false;
    return;
  }
  static bool pressed = false, waitRelease = false, moved = false;
  static int firstX = 0, firstY = 0;
  static uint32_t started = 0;
  bool down = touch.touched();
  if (!down) {
    if (pressed && !waitRelease && !moved && millis() - started < 1500) handleTouchTap(firstX, firstY);
    pressed = false; waitRelease = false; moved = false; return;
  }
  if (waitRelease) return;
  TS_Point p = touch.getPoint();
  if (!pressed) { pressed = true; moved = false; firstX = p.x; firstY = p.y; started = millis(); return; }
  if (millis() - started > 1500) { waitRelease = true; return; }
  int dx = p.x - firstX, dy = p.y - firstY;
  if (!touchMenuOpen && abs(dx) > 650 && abs(dx) > abs(dy)) {
    waitRelease = true;
    int next = constrain(activePage + (dx < 0 ? 1 : -1), 0, PAGE_COUNT - 1);
    if (next != activePage) { selectPage(next); drawBoard(); lastRefresh = 0; }
    return;
  }
  if (abs(dx) > 180 || abs(dy) > 180) moved = true; // kein Tippen bei Wischgeste
  // Ein Antippen wird erst beim Loslassen ausgeloest: Wischen bleibt priorisiert.
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
analogSetPinAttenuation(34, ADC_11db);
  pinMode(BACKLIGHT_PIN, OUTPUT);
  digitalWrite(BACKLIGHT_PIN, HIGH); // Always ON after power cycle, never persist sleep
  pinMode(33, OUTPUT); digitalWrite(33, HIGH); // Touch-CS inaktiv
  pinMode(5, OUTPUT); digitalWrite(5, HIGH);   // SD-CS inaktiv
  gfx->begin(27000000);
  setupTouch();
  loadTouchCalibration();
  gfx->setTextWrap(false);
  gfx->fillScreen(TRAM_COLOR_BLACK);
  drawLine("Bern Abfahrtstafel", 12, 20, TRAM_COLOR_AMBER);
  loadSettings();
  WiFi.mode(WIFI_STA);
  startWiFiAttempt(); // bez dlouheho blokovani dotyku a webove aplikace
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP("TRAMLI-BERN-2");
    fallbackAP=true;
    gfx->fillScreen(TRAM_COLOR_BLACK);
    drawLine("WLAN: TRAMLI-BERN-2", 12, 32, TRAM_COLOR_AMBER);
    drawLine("Browser: 192.168.4.1", 12, 70, TRAM_COLOR_WHITE);
  } else {
    configTzTime("CET-1CEST,M3.5.0,M10.5.0/3", "pool.ntp.org");
    statusText = "Lade Abfahrten";
    drawBoard();
  }
  configTzTime("CET-1CEST,M3.5.0,M10.5.0/3", "pool.ntp.org");
  setupRoutes();
  calibrateTouch();
  drawBoard();
}

void loop() {
  server.handleClient();
  wifiTick();
  checkSwipe();
  if((int32_t)(millis()-nextLightCheck)>=0) {
    nextLightCheck=millis()+30000UL;
    if(brightnessPwmEnabled && !displaySleeping)applyBacklight();
  }
  if (!displaySleeping && displayAutoOff && displayIdleMinutes &&
      millis()-displayLastActivity >= (uint32_t)displayIdleMinutes*60000UL) displaySleep();
  static bool wasConnected = false;
  bool connected = WiFi.status() == WL_CONNECTED;
  if (connected != wasConnected) {
    wasConnected = connected;
    lastRefresh = 0;
    if (!connected) {
      wifiLastIssue="Spojeni preruseno, zkousim ulozene site";
      if (!fallbackAP && !wifiAttemptPending) {
        WiFi.mode(WIFI_AP_STA);
        WiFi.softAP("TRAMLI-BERN-2");fallbackAP=true;
      }
    }
    if (!touchMenuOpen) drawBoard();
  }
  if (connected && (lastRefresh == 0 || millis() - lastRefresh >= REFRESH_MS)) {
    refreshBoard();
    lastRefresh = millis();
  }
  retryFailedStops();
  static uint32_t lastBlink = 0;
  if (millis() - lastBlink >= 500) {
    lastBlink = millis();
    if (!touchMenuOpen && !displaySleeping) { drawHeader(); if (connected && stopCount) drawDepartureRows(); }
  }
  static uint32_t lastWarning = 0;
  if (millis() - lastWarning >= 220) {
    lastWarning = millis();
  }
  delay(5);

}

