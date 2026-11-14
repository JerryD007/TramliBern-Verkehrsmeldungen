#include <atomic>
#include <esp_arduino_version.h>

#include <WiFi.h>
#include <esp_wifi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <Arduino_GFX_Library.h>
#include "BootIntro.h"
static BootIntro bootIntro;
#include <time.h>
#include <SPI.h>
#include <XPT2046_Touchscreen.h>

// Public CA trust anchors from the system trust store; no insecure TLS fallback.
static const char TRUST_ANCHORS[] PROGMEM = R"CERT(-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
MIICGzCCAaGgAwIBAgIQQdKd0XLq7qeAwSxs6S+HUjAKBggqhkjOPQQDAzBPMQsw
CQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJuZXQgU2VjdXJpdHkgUmVzZWFyY2gg
R3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBYMjAeFw0yMDA5MDQwMDAwMDBaFw00
MDA5MTcxNjAwMDBaME8xCzAJBgNVBAYTAlVTMSkwJwYDVQQKEyBJbnRlcm5ldCBT
ZWN1cml0eSBSZXNlYXJjaCBHcm91cDEVMBMGA1UEAxMMSVNSRyBSb290IFgyMHYw
EAYHKoZIzj0CAQYFK4EEACIDYgAEzZvVn4CDCuwJSvMWSj5cz3es3mcFDR0HttwW
+1qLFNvicWDEukWVEYmO6gbf9yoWHKS5xcUy4APgHoIYOIvXRdgKam7mAHf7AlF9
ItgKbppbd9/w+kHsOdx1ymgHDB/qo0IwQDAOBgNVHQ8BAf8EBAMCAQYwDwYDVR0T
AQH/BAUwAwEB/zAdBgNVHQ4EFgQUfEKWrt5LSDv6kviejM9ti6lyN5UwCgYIKoZI
zj0EAwMDaAAwZQIwe3lORlCEwkSHRhtFcP9Ymd70/aTSVaYgLXTWNLxBo1BfASdW
tL4ndQavEi51mI38AjEAi/V3bNTIZargCyzuFJ0nN6T5U6VR5CmD1/iQMVtCnwr1
/q4AaOeMSQ+2b1tbFfLn
-----END CERTIFICATE-----
)CERT";

// LCDWIKI E32R40T: ESP32-32E, ST7796, 320 x 480. Display im Querformat.
Arduino_DataBus *bus = new Arduino_ESP32SPI(2, 15, 14, 13, 12, HSPI);
Arduino_GFX *gfx = new Arduino_ST7796(bus, GFX_NOT_DEFINED, 1);
WebServer server(80);
SPIClass touchSPI(HSPI);
XPT2046_Touchscreen touch(33, 36);
bool touchReady = false;
int activePage = 0;
constexpr int STOPS_PER_PAGE = 3;
constexpr int PAGE_COUNT = 3;
Preferences prefs;

#ifdef BLACK
#undef BLACK
#endif
#ifdef WHITE
#undef WHITE
#endif
constexpr uint16_t BLACK = 0x0000, AMBER = 0xFDE0, WHITE = 0xFFFF;
// UI accent colours.
constexpr uint16_t BUTTON_RED = 0xF800;
constexpr uint16_t BUTTON_BLUE = 0x001F;
// Die Stadtbezeichnungen auf dem Display sind ASCII, die API-Suche nutzt den Ortsnamen.
constexpr int CITY_COUNT = 13;
static_assert(CITY_COUNT == IntroAssets::cityCount, "Intro city order/count must match CITY_LABELS");
const char *const CITY_LABELS[CITY_COUNT] = {
  "BERN", "ZUERICH", "BASEL", "GENF", "LAUSANNE", "LUZERN", "THUN",
  "ST.GALLEN", "WINTERTHUR", "BIEL", "FRIBOURG", "NEUENBURG", "ZUG"
};
const char *const CITY_QUERIES[CITY_COUNT] = {
  "Bern", "Zürich", "Basel", "Genève", "Lausanne", "Luzern", "Thun",
  "St. Gallen", "Winterthur", "Biel/Bienne", "Fribourg", "Neuchâtel", "Zug"
};
// Final approved city-menu design: white labels and city-colour underlines.
#include "MenuCrests.h"
const uint16_t CITY_ACCENTS[CITY_COUNT] = {
  AMBER, 0x045D, 0x045D, BUTTON_RED, BUTTON_RED, 0x045D, AMBER,
  AMBER, BUTTON_RED, BUTTON_RED, 0x045D, AMBER, 0x045D
};
int activeCity = 0;
uint16_t favoriteCities = 0;
uint8_t introMode = 2; // 0 off, 1 startup only, 2 startup and city changes.
uint8_t autoPageSeconds = 0;
uint8_t homeCity = 255, homePage = 0; // Unset until explicitly configured.
uint32_t autoPageAt = 0;
int cityForPosition(int position) {
  if(position<0 || position>=CITY_COUNT) return -1;
  int n=0;
  for(int group=1;group>=0;--group) for(int city=0;city<CITY_COUNT;++city)
    if(((favoriteCities>>city)&1)==group && n++==position) return city;
  return -1;
}
bool validAutoSeconds(int seconds) { return seconds>=0 && seconds<=60; }
void resetAutoPageTimer() { autoPageAt=millis(); }
void loadComfortSettings() {
  favoriteCities=prefs.getUShort("favorites",0)&0x1fff;
  introMode=prefs.getUChar("introMode",2);if(introMode>2)introMode=2;
  autoPageSeconds=prefs.getUChar("autoSecs",0);if(!validAutoSeconds(autoPageSeconds))autoPageSeconds=0;
  homeCity=prefs.getUChar("homeCity",255);homePage=prefs.getUChar("homePage",0);
  if(homeCity>=CITY_COUNT || homePage>=PAGE_COUNT){homeCity=255;homePage=0;}
  resetAutoPageTimer();
}
void saveComfortSettings() {
  prefs.putUShort("favorites",favoriteCities);
  prefs.putUChar("introMode",introMode);prefs.putUChar("autoSecs",autoPageSeconds);
  prefs.putUChar("homeCity",homeCity);prefs.putUChar("homePage",homePage);
  resetAutoPageTimer();
}
void goHome();
void serviceAutoPages();
void setupComfortRoutes();
bool numericArg(const char *name,int low,int high,int &out);
void touchButton(const String &label, int x, int y, int w, int h, uint16_t color);


constexpr uint32_t REFRESH_MS = 60000;
constexpr int MAX_STOPS = 9;
constexpr int DEPARTURES_PER_STOP = 3;
constexpr int PAGE3_DEPARTURES = 7;
constexpr int MAX_ROWS_PER_PAGE = 7;
constexpr int MAX_DEPARTURES = PAGE_COUNT * MAX_ROWS_PER_PAGE;
float batteryVoltage();
bool stopFetchFailed[MAX_STOPS] = {};
bool fetchFailed = false;
bool clockReady = false;
String lastHttpError;
String stopErrors[MAX_STOPS];
uint32_t lastSuccess[MAX_STOPS] = {};
uint32_t lastAttempt[MAX_STOPS] = {};
uint32_t retryDelay[MAX_STOPS] = {};
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
uint8_t pageSlots[PAGE_COUNT]={2,2,1};
int moveTargetPage=0;
bool slotEnabled(int slot) { return slot>=0 && slot<MAX_STOPS && slot%STOPS_PER_PAGE<pageSlots[slot/STOPS_PER_PAGE]; }
int pageEndSlot(int page) { return page*STOPS_PER_PAGE+pageSlots[page]; }
int pageRows(int page) { const int rows[]={0,7,3,2}; return rows[pageSlots[page]]; }
int stationHeaderY(int page,int position) { const int stride[]={0,220,110,72};return 43+position*stride[pageSlots[page]]; }
int departureSpacing(int page) { return pageSlots[page]==1?27:pageSlots[page]==2?28:24; }
String layoutKey() { return "layout"+String(activeCity); }
bool decodeLayout(const String &data,uint8_t *counts,String *items) {
  if(data.length()<6 || !data.startsWith("FP1"))return false;
  for(int p=0;p<PAGE_COUNT;++p){if(data[3+p]<'1'||data[3+p]>'3')return false;counts[p]=data[3+p]-'0';}
  unsigned cursor=6;
  for(int i=0;i<MAX_STOPS;++i){
    unsigned len=0,begin=cursor;
    while(cursor<data.length() && data[cursor]>='0' && data[cursor]<='9'){len=len*10+data[cursor++]-'0';if(len>1000)return false;}
    if(cursor==begin||cursor>=data.length()||data[cursor++]!=':'||cursor+len>data.length())return false;
    items[i]=data.substring(cursor,cursor+len);cursor+=len;
  }
  return cursor==data.length();
}
void saveStops();
void setPageSlots(int page,int count);

int stopCount = 0;
String wifiSsid, wifiPassword;
constexpr const char *VERSION = "TramliBern 4.0 RC26 HomeCross";
struct WifiProfile { char ssid[33]; char pass[64]; uint8_t worked; };
WifiProfile networks[4] = {};
int wifiTrying = -1, wifiPreferred = 0;
uint32_t wifiAttemptAt = 0;
bool wifiAttempting = false, accessPoint = false;
int brightness = 100, nightBrightness = 15, nightStart = 22*60, nightEnd = 7*60;
bool nightEnabled = false, pwmReady = false;
int wifiEdit = -1, keyboardPage = 0, moveSource = -1;
String pendingSsid, inputText;
String menuNotice;
struct NearbyWifi { String ssid; int rssi; bool open; };
NearbyWifi nearbyWifi[16];
int nearbyCount=0, nearbyPage=0;
bool wifiScanning=false, wifiScanDone=false, wifiScanError=false;
uint32_t wifiScanAt=0;
void startWifiScan(bool force=false);
void serviceWifiScan();
String wifiScanJson();
// Manufacturer schematic: R2=100k, R3=100k, BAT_ADC -> GPIO34.
// Measured BAT+ voltage does not prove physical battery presence; no percentage estimate.
bool batteryEnabled = true;
bool batteryWarningEnabled = false;
constexpr bool BATTERY_VERIFIED = true; // Board schematic verified, individual unit not calibrated.
constexpr float BATTERY_DIVIDER = 2.0f;
int batteryLowMv = 0; // 0 = unset; user-selected threshold, no assumed battery chemistry.
void loadExtendedSettings();
void serviceWifi();
void applyBrightness();
void drawExtendedMenu();
bool extendedTap(int x, int y);
void setupExtendedRoutes();

String statusText = "Startet";
uint32_t lastRefresh = 0;
struct Departure {
  String line, destination, station;
  bool tram = false;
  time_t when;
};
Departure departures[MAX_DEPARTURES];
int departureCount = 0;
struct DepartureJob {
  String url, error;
  JsonDocument doc;
  int index = -1;
  uint32_t generation = 0;
  bool ok = false;
};
DepartureJob fetchJob;
std::atomic<bool> fetchBusy{false}, fetchReady{false};
bool fetchQueued[MAX_STOPS] = {};
uint32_t dataGeneration = 0;
void serviceDepartures();
bool getJson(const String &url, JsonDocument &doc) {
  // Searches run on the UI thread only after the departure worker releases TLS RAM.
  while (fetchBusy.load()) delay(5);
  return getJsonImpl(url, doc, lastHttpError);
}



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

bool getJsonImpl(const String &url, JsonDocument &doc, String &errorText) {
  errorText = "";
  doc.clear();
  WiFiClientSecure client;
  client.setCACert(TRUST_ANCHORS); // Verify server certificate; never fall back to insecure TLS.
  HTTPClient http;
  http.setReuse(false);
  http.setConnectTimeout(4000);
  http.setTimeout(4000);
  if (!http.begin(client, url)) { errorText = "HTTP Start fehlgeschlagen"; return false; }
  http.addHeader("Accept", "application/json");
  http.addHeader("Accept-Encoding", "identity");
  int code = http.GET();
  int expected = http.getSize();
  Serial.println("URL: " + url);
  if (code != HTTP_CODE_OK) {
    errorText = "HTTP " + String(code) + ": " + http.errorToString(code);
    http.end(); return false;
  }
  if (expected > (int)JsonResponseBuffer::CAPACITY) {
    errorText = "Antwort zu gross: " + String(expected) + " Bytes; Feldfilter pruefen";
    http.end(); return false;
  }
  JsonResponseBuffer response;
  if (!response.data) {
    errorText = "Empfangspuffer nicht verfuegbar; Heap " + String(ESP.getFreeHeap());
    http.end(); return false;
  }
  // Dieser Weg nutzt die HTTPClient-Behandlung fuer Laenge und Chunked-Encoding.
  int received = http.writeToStream(&response);
  http.end();
  Serial.printf("HTTP %d | Transfer %d | Bytes %u | erwartet %d | Heap %u\n",
    code, received, (unsigned)response.length, expected, (unsigned)ESP.getFreeHeap());
  if (received < 0 || response.overflow || (expected >= 0 && response.length != (size_t)expected)) {
    errorText = "Transfer " + String(received) + ": " + http.errorToString(received) +
                   " | gelesen " + String((unsigned)response.length) + " | erwartet " + String(expected);
    return false;
  }
  if (!response.length) { errorText = "HTTP 200, aber Antwort leer"; return false; }
  // const char* erzwingt eigene JSON-Strings: der Empfangspuffer wird danach freigegeben.
  DeserializationError error = deserializeJson(doc, (const char *)response.data, response.length);
  if (error) errorText = "JSON: " + String(error.c_str()) + " | Bytes " + String((unsigned)response.length);
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
void selectPage(int page, bool persist = true) {
  resetAutoPageTimer();
  if (page < 0 || page >= PAGE_COUNT) return;
  if(page==activePage){if(persist && prefs.getUChar(cityPageKey(activeCity).c_str(),0)!=page)prefs.putUChar(cityPageKey(activeCity).c_str(),page);return;}
  activePage = page;
  if(persist) prefs.putUChar(cityPageKey(activeCity).c_str(), (uint8_t)page);
}
void loadCityStops() {
  uint8_t counts[PAGE_COUNT];String items[MAX_STOPS];
  bool migrated=decodeLayout(prefs.getString(layoutKey().c_str(),""),counts,items);
  for(int page=0;page<PAGE_COUNT;++page)pageSlots[page]=migrated?counts[page]:(page==2?1:2);
  for(int i=0;i<MAX_STOPS;++i){stops[i]=migrated?items[i]:String("");retryDelay[i]=15000;}
  if(!migrated) {
    const int mapped[]={0,1,3,4,6};
    int oldCount=prefs.getUChar(cityCountKey(activeCity).c_str(),0);
    bool fixed=prefs.getBool("fixedSlots",false);
    for(int i=0;i<5;++i)if(fixed || i<oldCount)stops[mapped[i]]=prefs.getString(cityStopKey(activeCity,i).c_str(),"");
  }
  stopCount=0;for(int i=0;i<MAX_STOPS;++i)if(slotEnabled(i)&&stops[i].length())++stopCount;
}
void resetStopData() {
  ++dataGeneration;
  for (bool &queued : fetchQueued) queued = false;
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
  activeCity = prefs.getUChar("city", 0);
  if (activeCity < 0 || activeCity >= CITY_COUNT) activeCity = 0;
  loadCityStops();
  restoreCityPage();
  loadExtendedSettings();
  loadComfortSettings();
}
void saveStops() {
  stopCount=0;for(int i=0;i<MAX_STOPS;++i)if(slotEnabled(i)&&stops[i].length())++stopCount;
  resetStopData();
  String encoded="FP1";for(int p=0;p<PAGE_COUNT;++p)encoded+=char('0'+pageSlots[p]);
  for(int i=0;i<MAX_STOPS;++i)encoded+=String(stops[i].length())+":"+stops[i];
  String key=layoutKey();prefs.putString(key.c_str(),encoded);
  if(prefs.getString(key.c_str(),"")!=encoded){menuNotice="Speichern fehlgeschlagen - Speicher voll?";Serial.println(menuNotice);}
  else menuNotice="";
}
void setPageSlots(int page,int count) {
  if(page<0||page>=PAGE_COUNT||count<1||count>3||pageSlots[page]==count)return;
  pageSlots[page]=count;saveStops();resetAutoPageTimer();lastRefresh=0;++touchExitRevision;
}
// Never silently fall back to a different page when its places are occupied.
int firstFreeSlotOnPage(int page) {
  if (page < 0 || page >= PAGE_COUNT) return -1;
  for (int slot = page * STOPS_PER_PAGE; slot < pageEndSlot(page); ++slot)
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
  resetAutoPageTimer();
  activeCity = city;
  prefs.putUChar("city", activeCity);
  loadCityStops();
  restoreCityPage();
  resetStopData();
  lastRefresh = 0;
  statusText = "Lade Abfahrten";
  // Both touch and Safari use this path. Saving happens before the animation.
  touchMenuOpen = false;
  touchMode = 0;
  touchEditIndex = -1;
  if(introMode==2) bootIntro.begin(gfx, millis(), activeCity);
  else { bootIntro.cancel(); ++touchExitRevision; drawBoard(); }
}
void goHome() {
  if(homeCity>=CITY_COUNT || homePage>=PAGE_COUNT) return;
  changeCity(homeCity);
  selectPage(homePage);
  touchMenuOpen=false;touchMode=0;touchEditIndex=-1;
  ++touchExitRevision;lastRefresh=0;drawBoard();
}
bool pageHasStops(int page) {
  if(page<0 || page>=PAGE_COUNT)return false;
  for(int i=page*STOPS_PER_PAGE;i<pageEndSlot(page);++i)
    if(stops[i].length())return true;
  return false;
}
void serviceAutoPages() {
  uint32_t now=millis();
  if(!autoPageSeconds || touchMenuOpen || bootIntro.active()){autoPageAt=now;return;}
  if(uint32_t(now-autoPageAt)<uint32_t(autoPageSeconds)*1000)return;
  autoPageAt=now;
  for(int step=1;step<PAGE_COUNT;++step) {
    int page=(activePage+step)%PAGE_COUNT;
    if(pageHasStops(page)){selectPage(page,false);lastRefresh=0;drawBoard();return;}
  }
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
  // Built-in font is byte-based: render PLÄTZE as PLATZE with explicit umlaut.
  int umlaut=s.indexOf("PLATZE");
  if(umlaut>=0){
    gfx->fillRect(x+(umlaut*6+13)*size,y-size,size,size,color);
    gfx->fillRect(x+(umlaut*6+15)*size,y-size,size,size,color);
  }
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
  gfx->drawRoundRect(x, y + 3, 34, 13, 2, AMBER);
  gfx->drawFastHLine(x + 2, y + 4, 30, AMBER);
  gfx->drawFastHLine(x + 2, y + 13, 30, AMBER);
  gfx->drawLine(x, y + 7, x + 2, y + 5, AMBER);
  gfx->drawLine(x, y + 11, x + 2, y + 13, AMBER);
  gfx->drawFastHLine(x + 1, y + 11, 5, AMBER);
  gfx->drawRect(x + 2, y + 6, 5, 5, AMBER);
  gfx->drawRect(x + 8, y + 6, 7, 5, AMBER);
  gfx->drawFastVLine(x + 16, y + 5, 9, AMBER);
  gfx->drawFastVLine(x + 19, y + 5, 9, AMBER);
  gfx->drawRect(x + 20, y + 6, 8, 5, AMBER);
  gfx->drawRect(x + 28, y + 6, 3, 5, AMBER);
  gfx->drawRect(x + 32, y + 6, 1, 5, AMBER);
  gfx->drawFastHLine(x + 6, y + 11, 9, AMBER);
  gfx->drawFastHLine(x + 20, y + 11, 8, AMBER);
  gfx->fillCircle(x + 8, y + 16, 2, AMBER);
  gfx->fillCircle(x + 25, y + 16, 2, AMBER);
  gfx->fillCircle(x + 8, y + 16, 1, BLACK);
  gfx->fillCircle(x + 25, y + 16, 1, BLACK);
}

void drawTramIcon(int x, int y) {
  gfx->drawRoundRect(x, y + 5, 34, 12, 2, AMBER);
  gfx->drawFastHLine(x + 3, y + 5, 28, AMBER);
  gfx->drawLine(x + 14, y + 5, x + 18, y, AMBER);
  gfx->drawLine(x + 18, y, x + 22, y + 5, AMBER);
  gfx->drawFastHLine(x + 15, y, 7, AMBER);
  gfx->drawRect(x + 3, y + 8, 5, 5, AMBER);
  gfx->drawRect(x + 9, y + 8, 5, 5, AMBER);
  gfx->drawRect(x + 16, y + 7, 5, 9, AMBER);
  gfx->drawFastVLine(x + 18, y + 7, 9, AMBER);
  gfx->drawRect(x + 23, y + 8, 4, 5, AMBER);
  gfx->drawRect(x + 28, y + 8, 4, 5, AMBER);
  gfx->fillCircle(x + 6, y + 18, 1, AMBER);
  gfx->fillCircle(x + 11, y + 18, 1, AMBER);
  gfx->fillCircle(x + 25, y + 18, 1, AMBER);
  gfx->fillCircle(x + 30, y + 18, 1, AMBER);
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
  gfx->setTextColor(color, BLACK);
  gfx->setCursor(x, y);
  gfx->print(text);
}

// Native-size antialiased city crests, shared by city selection and board heading.
void drawCityCrest(int city, int x, int y) {
  MenuCrests::draw(gfx, city, x, y);
}
void drawCityBadge(int city, int x, int y, int w, int h, int size, bool selected) {
  if(city<0 || city>=CITY_COUNT) return;
  gfx->fillRect(x,y,w,h,BLACK); // No card outline or light crest backing.
  drawCityCrest(city,x+6,y+(h-26)/2);
  int textX=x+41;
  int textY=y+(h-8*size)/2-1;
  if(city==1 && textY<y+size) textY=y+size;
  // The built-in bitmap font is byte-based: draw U and its umlaut explicitly.
  drawLine(city==1 ? "ZURICH" : CITY_LABELS[city],textX,textY,WHITE,size);
  if(city==1) {
    gfx->fillRect(textX+7*size,textY-size,size,size,WHITE);
    gfx->fillRect(textX+9*size,textY-size,size,size,WHITE);
  }
  gfx->fillRect(textX,y+h-5,w-49,2,CITY_ACCENTS[city]);
  // Selection marker appears only in the city list, without changing its background.
  if(selected) {
    int mx=x+w-19, my=y+(h-14)/2;
    gfx->fillRect(mx,my,14,14,BUTTON_RED);
    gfx->fillRect(mx+3,my+6,8,2,WHITE);
    gfx->fillRect(mx+6,my+3,2,8,WHITE);
  }
}
// Informational heading, same selected-city badge as the city menu.
void drawCityButton() {
  gfx->fillRect(112,2,285,30,BLACK);
  drawCityBadge(activeCity,112,2,285,30,3,false);
}

void drawHeader() {
  if (bootIntro.active()) return;
  static String previousClock;
  static int previousFill = -2;
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
    drawOpaque(clock, 410, 8, 5, WHITE);
    previousClock = clock;
  }
  if (force || millis() - batteryRead >= 10000) {
    batteryRead = millis();
    float volt = batteryVoltage();
    int fill = !batteryEnabled ? -3 : volt > 0 ? (int)(volt * 100 + 0.5f) * 2 + (isLowBattery(volt, BATTERY_VERIFIED, batteryLowMv / 1000.0f) ? 1 : 0) : -1;
    static int previousColor = -1;
    int visualColor = batteryVisualColor(volt);
    if (force || fill != previousFill || visualColor != previousColor) {
      previousColor = visualColor;
      gfx->fillRect(10, 10, 96, 18, BLACK);
      if (batteryEnabled) {
      uint16_t color = batteryVisualColor(volt);
      gfx->drawRect(10, 12, 20, 12, color);
      gfx->fillRect(31, 15, 3, 6, color);
      int barWidth = batteryBarWidth(volt);
      if (barWidth) gfx->fillRect(12, 14, barWidth, 8, color);
      drawLine(fill < 0 ? "--" : String(volt, 2) + " V", 43, 14, color, 1);
      }
      previousFill = fill;
    }
  }
}

// Auf Seite 1 und 2 wie bisher: je zwei Haltestellen, drei Fahrten.
// Auf Seite 3: genau eine Haltestelle mit sieben Fahrten.
void drawDepartureRows() {
  if (bootIntro.active()) return;
  time_t now = time(nullptr);
  const int stationsOnPage = pageSlots[activePage];
  const int rowsPerStation = pageRows(activePage);
  const int spacing = departureSpacing(activePage);
  for (int position = 0; position < stationsOnPage; ++position) {
    int station = activePage * STOPS_PER_PAGE + position;
    const int headerY = stationHeaderY(activePage,position);
    const int baseY = headerY+23;
    const int cacheBase = position * rowsPerStation;
    if (!stops[station].length()) {
      drawOpaque("Haltestelle noch nicht eingerichtet", 12, headerY, 38, WHITE);
      for (int slot = 0; slot < rowsPerStation; ++slot) {
        int y = baseY + slot * spacing;
        drawOpaque("", 14, y, 32, AMBER);
        gfx->fillRect(398, y - 3, 74, 24, BLACK);
      }
      continue;
    }
    String stationTitle = stops[station];
    stationTitle.replace(String(CITY_QUERIES[activeCity]) + ", ", "");
    stationTitle = clip(stationTitle, stopFetchFailed[station] ? 29 : 38);
    if (stopFetchFailed[station]) stationTitle += " (Fehler)";
    if (stationTitle != stationCache[position]) {
      drawOpaque(stationTitle, 12, headerY, 38, WHITE);
      stationCache[position] = stationTitle;
    }
    int selected[PAGE3_DEPARTURES];
    int shown = 0;
    for (int i = 0; i < departureCount && shown < rowsPerStation; ++i) {
      if (departures[i].station != stops[station]) continue;
      if (lastSuccess[station] && millis() - lastSuccess[station] > 180000) continue;
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
        if (stopFetchFailed[station] || (lastSuccess[station] && millis() - lastSuccess[station] > 90000)) timing = "alt";
        else if (!clockReady) timing = "--";
        else {
          long minutes = max(0L, (long)((d->when - now + 59) / 60));
          if (minutes == 0) timing = (millis() / 500) % 2 == 0 ?
              (d->tram ? "ICON_T" : "ICON_B") : "";
          else timing = minutes > 999 ? String("999+") : String(minutes) + "'";
        }
      } else if (slot == 0) {
        content = stopFetchFailed[station] ? "Datenabruf fehlgeschlagen" : "Keine Abfahrten verfuegbar";
      }
      if (content != rowCache[cache]) {
        if (d) {
          if (rowCache[cache].indexOf('|') < 0) drawOpaque("", 14, y, 32, AMBER);
          drawOpaque(clip(d->line, 6), 14, y, 6, AMBER);
          drawOpaque(clip(d->destination, 22), 100, y, 24, AMBER);
        } else {
          drawOpaque(content, 14, y, 32, WHITE);
        }
        rowCache[cache] = content;
      }
      if (timing != timeCache[cache]) {
        gfx->fillRect(398, y - 3, 74, 24, BLACK);
        if (timing == "ICON_T") drawTramIcon(400, y - 2);
        else if (timing == "ICON_B") drawBusIcon(400, y - 2);
        else drawOpaque(timing, 400, y, 6, AMBER);
        timeCache[cache] = timing;
      }
    }
  }
}

void drawBoard() {
  if (bootIntro.active()) return;
  if (touchMenuOpen) return;
  static String previousLayout;
  String layout = String(activeCity) + ":" + String(WiFi.status() == WL_CONNECTED) + ":" + String(stopCount) + ":" + String(activePage) + ":" + String(touchExitRevision)+":"+String(pageSlots[activePage]);
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
  gfx->fillScreen(BLACK);
  gfx->drawFastHLine(10, 34, 460, AMBER);
  drawHeader();
  if (WiFi.status() != WL_CONNECTED) {
    drawLine("WLAN nicht verbunden", 12, 60, WHITE);
    drawLine("Setup: TRAMLI-BERN-2", 12, 93, AMBER);
    drawLine("Browser: 192.168.4.1", 12, 123, WHITE);
  } else {
    for (int position = 0; position < pageSlots[activePage]; ++position) {
      int i = activePage * STOPS_PER_PAGE + position;
      if (!stops[i].length()) continue;
      String name = stops[i];
      name.replace(String(CITY_QUERIES[activeCity]) + ", ", "");
      drawLine(clip(name, 38), 12, stationHeaderY(activePage,position), WHITE, 2);
    }
    drawDepartureRows();
  }
  gfx->drawFastHLine(10, 267, 460, AMBER);
  String address = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
  gfx->fillRoundRect(12, 270, 100, 25, 4, BUTTON_RED);
  gfx->drawRoundRect(12, 270, 100, 25, 4, AMBER);
  drawLine("MENU", 38, 275, WHITE, 2);
  drawLine(address, 126, 278, WHITE, 1);
  if(homeCity<CITY_COUNT) {
    gfx->fillRoundRect(254,270,60,25,4,BUTTON_RED);
    gfx->drawRoundRect(254,270,60,25,4,WHITE);
    // Centered white cross; keep the existing Home touch target unchanged.
    gfx->fillRect(281,275,5,15,WHITE);
    gfx->fillRect(276,280,15,5,WHITE);
  }
  for (int page = 0; page < PAGE_COUNT; ++page) {
    int x = 320 + page * 50;
    gfx->fillRect(x, 270, 46, 24, page == activePage ? AMBER : BLACK);
    gfx->drawRect(x, 270, 46, 24, AMBER);
    drawLine(String(page + 1), x + 17, 274, page == activePage ? BLACK : WHITE, 2);
  }
  gfx->drawFastHLine(10, 299, 460, AMBER);
}

// Dokumentiertes ISO-8601-Feld prognosis.departure in Unixzeit umrechnen.
// Expliziter UTC-Offset vermeidet Probleme bei Sommer-/Winterzeit.
int64_t parseDepartureTime(const char *value) {
  if (!value || !*value) return 0;
  if (strlen(value) < 20 || value[4]!='-' || value[7]!='-' || value[10]!='T' || value[13]!=':' || value[16]!=':') return 0;
  for (int p=0;p<19;++p) if (p!=4&&p!=7&&p!=10&&p!=13&&p!=16 && (value[p]<'0'||value[p]>'9')) return 0;
  int year, month, day, hour, minute, second;
  if (sscanf(value, "%d-%d-%dT%d:%d:%d", &year, &month, &day, &hour, &minute, &second) != 6) return 0;
  if (strlen(value) < 20 || month < 1 || month > 12 || day < 1 || day > 31) return 0;
  if (year < 1970 || year > 2199 || hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59) return 0;
  const int monthDays[] = {31,28,31,30,31,30,31,31,30,31,30,31};
  int daysInMonth = monthDays[month-1] + (month == 2 && year%4 == 0 && (year%100 != 0 || year%400 == 0));
  if (day > daysInMonth) return 0;
  int offsetHour = 0, offsetMinute = 0, offset = 0;
  size_t length = strlen(value);
  if (value[19] == '+' || value[19] == '-') {
    int minutePos = length == 25 && value[22] == ':' ? 23 : length == 24 ? 22 : -1;
    if (minutePos < 0) return 0;
    for (int p : {20,21,minutePos,minutePos+1}) if (value[p] < '0' || value[p] > '9') return 0;
    offsetHour = (value[20]-'0')*10 + value[21]-'0';
    offsetMinute = (value[minutePos]-'0')*10 + value[minutePos+1]-'0';
    if (offsetHour > 23 || offsetMinute > 59) return 0;
    offset = (offsetHour * 60 + offsetMinute) * 60;
    if (value[19] == '-') offset = -offset;
  } else if (value[19] != 'Z' || length != 20) return 0;
  year -= month <= 2;
  int era = (year >= 0 ? year : year - 399) / 400;
  unsigned yoe = (unsigned)(year - era * 400);
  unsigned doy = (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
  unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  int64_t days = (int64_t)era * 146097 + doe - 719468;
  return days * 86400 + hour * 3600 + minute * 60 + second - offset;
}

// Jeder Halt wird separat aktualisiert. Ein Fehler loescht nicht den letzten Empfang.
String stationboardUrl(int index) {
  return "https://transport.opendata.ch/v1/stationboard?station=" +
               urlEncode(stops[index]) + "&limit=" + String(pageRows(index/STOPS_PER_PAGE)*2) +
               "&transportations%5B%5D=tram&transportations%5B%5D=bus"
               "&fields%5B%5D=stationboard/category"
               "&fields%5B%5D=stationboard/number"
               "&fields%5B%5D=stationboard/to"
               "&fields%5B%5D=stationboard/stop/departureTimestamp"
               "&fields%5B%5D=stationboard/stop/prognosis/departure"
               "&fields%5B%5D=stationboard/stop/prognosis/departureTimestamp";
}
void applyStopResponse(int index, JsonDocument &doc, bool success, const String &errorText) {
  if (!success || !doc["stationboard"].is<JsonArray>()) {
    bool alreadyFailed = stopFetchFailed[index];
    stopFetchFailed[index] = true;
    stopErrors[index] = errorText.length() ? errorText : "Antwort ohne stationboard-Liste";
    retryDelay[index] = alreadyFailed ? min((uint32_t)60000, retryDelay[index] * 2) : 15000;
    // Abstand ab Ende des fehlgeschlagenen Abrufs messen.
    lastAttempt[index] = millis();
    Serial.println(stops[index] + ": " + stopErrors[index]);
    return;
  }
  Departure fresh[PAGE3_DEPARTURES];
  const int wanted = pageRows(index/STOPS_PER_PAGE);
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
    Departure &d = fresh[count];
    d.station = stops[index];
    String number = String(row["number"] | "");
    bool isTram = category == "T" || category == "TRAM";
    if (!category.length()) continue; // Do not guess vehicle type from a line number.
    d.tram = isTram;
    d.line = number;
    d.destination = String(row["to"] | "");
    d.when = (time_t)timestamp;
    ++count;
  }
  int keep = 0;
  for (int i = 0; i < departureCount; ++i) {
    bool stillSelected = false;
    for (int j = 0; j < MAX_STOPS; ++j) if (slotEnabled(j) && departures[i].station == stops[j]) stillSelected = true;
    if (stillSelected && departures[i].station != stops[index]) departures[keep++] = departures[i];
  }
  departureCount = keep;
  for (int i = 0; i < count && departureCount < MAX_DEPARTURES; ++i) departures[departureCount++] = fresh[i];
  stopFetchFailed[index] = false;
  stopErrors[index] = "";
  lastSuccess[index] = millis();
  retryDelay[index] = 15000;
}

// Synchronous adapter retained for host tests; production uses the worker below.
void fetchStop(int index) {
  lastAttempt[index] = millis();
  JsonDocument doc;
  bool ok = getJson(stationboardUrl(index), doc);
  applyStopResponse(index, doc, ok, lastHttpError);
}
void departureWorker(void *) {
  fetchJob.ok = getJsonImpl(fetchJob.url, fetchJob.doc, fetchJob.error);
  fetchReady.store(true);
  fetchBusy.store(false);
  vTaskDelete(nullptr);
}
void serviceDepartures() {
  if(fetchBusy.load()) return;
  if(fetchReady.load()) {
    if(fetchJob.generation == dataGeneration) {
      applyStopResponse(fetchJob.index, fetchJob.doc, fetchJob.ok, fetchJob.error);
      updateFetchStatus();
    }
    fetchJob.doc.clear(); fetchJob.error=""; fetchJob.url="";
    fetchReady.store(false);
  }
  if(touchMenuOpen || WiFi.status()!=WL_CONNECTED) return;
  for(int i=0;i<MAX_STOPS;++i) {
    if(i/STOPS_PER_PAGE != activePage || !slotEnabled(i)) {fetchQueued[i]=false;continue;}
    if(!fetchQueued[i] || !stops[i].length()) continue;
    fetchQueued[i]=false;
    if(lastAttempt[i] && millis()-lastAttempt[i]<15000) continue;
    fetchJob.url=stationboardUrl(i);fetchJob.index=i;fetchJob.generation=dataGeneration;
    lastAttempt[i]=millis();fetchBusy.store(true);
    if(xTaskCreate(departureWorker,"departures",10240,nullptr,1,nullptr)!=pdPASS) {
      fetchBusy.store(false);fetchJob.ok=false;fetchJob.error="Nicht genug RAM fuer Datenabruf";fetchReady.store(true);
    }
    break;
  }
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
  for (int i = activePage * STOPS_PER_PAGE; i < pageEndSlot(activePage); ++i) {
    // Auch regulaere Aktualisierungen beachten den Fehler-Abstand.
    if (stopFetchFailed[i] && millis() - lastAttempt[i] < retryDelay[i]) continue;
    if (stops[i].length() && (!lastAttempt[i] || millis() - lastAttempt[i] >= 15000)) fetchQueued[i] = true;
  }
  updateFetchStatus();
}

void retryFailedStops() {
  if (WiFi.status() != WL_CONNECTED) return;
  for (int i = activePage * STOPS_PER_PAGE; i < pageEndSlot(activePage); ++i) {
    if (stopFetchFailed[i] && millis() - lastAttempt[i] >= retryDelay[i]) {
      if (stops[i].length() && (!lastAttempt[i] || millis() - lastAttempt[i] >= 15000)) fetchQueued[i] = true;
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
input[type=checkbox]{display:inline-block;width:24px;min-height:24px;height:24px;margin:0 12px 0 0;vertical-align:middle}</style></head><body><header><div><b>▤ Trämli Bern</b><span>DEINE ABFAHRTSTAFELN</span></div></header><main>)TRAMLI";
  html += "<h1>" + escapeHtml(title) + "</h1><p class='muted'>" + escapeHtml(boardLabel()) + " · " + escapeHtml(currentAddress()) + "</p>";
  if (title != "Menü") html += "<nav aria-label='Navigation'><a href='/'>← Menü</a><a href='/boards'>▣ Meine Boards</a></nav>";
  return html;
}
String pageEnd() { return "<footer>Trämli Bern · Lokale Bedienung · Version 4.0 RC26 HomeCross · 3 Seiten · Je Seite 1–3 Haltestellen</footer></main></body></html>"; }
void sendPage(String html, int code = 200) {
  server.sendHeader("Cache-Control", "no-store");
  server.send(code, "text/html; charset=utf-8", html + pageEnd());
}
void showMessage(const String &message, int code = 400) {
  sendPage(pageStart("Hinweis") + "<section class='card'><p>" + escapeHtml(message) + "</p><a class='button' href='/'>Zurück zur Bedienung</a></section>", code);
}
void redirectHome() { server.sendHeader("Location", "/stops"); server.send(303); }
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
  server.on("/page-layout",HTTP_POST,[](){int page,count;if(!numericArg("page",0,PAGE_COUNT-1,page)||!numericArg("count",1,STOPS_PER_PAGE,count)){showMessage("Ungueltige Seitenbelegung");return;}setPageSlots(page,count);if(touchMenuOpen){if(touchMode==4)drawCityMenu();else drawExtendedMenu();}else drawBoard();redirectHome();});
  // Local configuration routes; no traffic-alert processing.
  server.on("/stops", HTTP_GET, []() {
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
      if (stopFetchFailed[i]) html += "<p class='notice'>" + escapeHtml(stops[i]) + ": " + escapeHtml(stopHealth(i)) + "</p>";
    }
    html += "<p class='status'>Anzeige: " + escapeHtml(statusText) + "</p><p class='notice'>" + escapeHtml(warningText()) + "</p>";
    if(menuNotice.length())html+="<p class='notice'>"+escapeHtml(menuNotice)+"</p>";
    html += "<section class='card'><h2>Displayseiten verwalten</h2><p class='hint'>Jede Seite: 1–3 Haltestellen. 1: sieben Abfahrten; 2: je drei; 3: je zwei. Bei weniger Plätzen bleiben ausgeblendete Haltestellen gespeichert. Die Position einer Haltestelle waehlen und auf Verschieben tippen. Falls der Platz belegt ist, werden die beiden Haltestellen getauscht.</p>";
    for (int page = 0; page < PAGE_COUNT; ++page) {
      html += "<h2>Seite " + String(page + 1) + (activePage == page ? " (aktiv)" : "") + "</h2>";
      html += "<form action='/page' method='post'><input type='hidden' name='page' value='" + String(page) + "'><button class='subtle'>Diese Seite anzeigen</button></form>";
      html += "<form action='/page-layout' method='post'><input type='hidden' name='page' value='"+String(page)+"'><label>Anzahl Haltestellen</label><select name='count'>";
      for(int n=1;n<=3;++n)html+="<option value='"+String(n)+"'"+String(pageSlots[page]==n?" selected":"")+">"+String(n)+"</option>";
      html+="</select><button>Speichern</button></form><ol>";
      for (int pos = 0; pos < pageSlots[page]; ++pos) {
        int index = page * STOPS_PER_PAGE + pos;
        if (!stops[index].length()) {
          html += "<li><span>Platz " + String(pos + 1) + ": Frei</span></li>";
          continue;
        }
        html += "<li><div><b>" + escapeHtml(stops[index]) + "</b><small class='muted'> · " + String(pageRows(page)) + " Abfahrten</small>";
        html += "<form action='/search' method='get'><input type='hidden' name='slot' value='" + String(index) + "'><label>Neue Haltestelle</label><input name='q' minlength='2' maxlength='100' required><button>Ersetzen suchen</button></form>";
        html += "<form action='/move' method='post'><input type='hidden' name='index' value='" + String(index) + "'>";
        html += "<label>Position</label><select name='slot' style='width:100%;padding:10px;margin:7px 0'>";
        for (int target = 0; target < MAX_STOPS; ++target) {
          if(!slotEnabled(target))continue;
          html += "<option value='" + String(target) + "'" + (index == target ? " selected" : "") + ">Seite " + String(target / STOPS_PER_PAGE + 1) + ", Platz " + String(target % STOPS_PER_PAGE + 1) + "</option>";
        }
        html += "</select><button class='subtle'>Verschieben / Tauschen</button></form>";
        html += "<form action='/remove' method='post'><input type='hidden' name='index' value='" + String(index) + "'><button class='subtle'>Entfernen</button></form></div></li>";
      }
      html += "</ol>";
    }
    if (!stopCount) html += "<p>Noch keine Haltestelle ausgewaehlt.</p>";
    html += "<p class='hint'>" + String(stopCount) + " aktive Haltestellen ausgewaehlt (bis zu 9). Jeder Platz ist frei waehlbar. Leere Seiten bleiben leer.</p>";
    html += "<form action='/refresh' method='post'><button class='subtle'>Anzeige aktualisieren</button></form></section>";
    html += "<section class='card'><h2>Haltestelle hinzufügen</h2>";
    if (stopCount >= MAX_STOPS) html += "<p class='notice'>Alle neun Plätze sind belegt. Nutze die Suche zum Ersetzen direkt bei der gewünschten Position.</p>";
    else {
      int target = firstFreeSlotOnPage(activePage);
      html += "<form action='/search' method='get'><label>Zielplatz</label><select name='slot' required>";
      html += "<option value=''" + String(target < 0 ? " selected" : "") + " disabled>Bitte freien Platz wählen</option>";
      for (int slot = 0; slot < MAX_STOPS; ++slot) if (slotEnabled(slot) && !stops[slot].length())
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
      if(!numericArg("slot",0,MAX_STOPS-1,target)||!slotEnabled(target)){showMessage("Bitte einen aktiven Zielplatz wählen.");return;}
    }
    if (target < 0) { showMessage("Bitte einen Zielplatz wählen."); return; }
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
          else {
            html += "<form action='/add' method='post'><input type='hidden' name='name' value='" + escapeHtml(name) + "'><label>Zielplatz</label><select name='slot'>";
            for (int slot = 0; slot < MAX_STOPS; ++slot) if (slot == target)
              html += "<option value='" + String(slot) + "'" + (slot == target ? " selected" : "") + ">Seite " + String(slot / STOPS_PER_PAGE + 1) + ", Platz " + String(slot % STOPS_PER_PAGE + 1) + "</option>";
            html += "</select><input type='hidden' name='expected' value='" + escapeHtml(stops[target]) + "'><button>Speichern / Ersetzen</button></form>";
          }

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

    String position = server.arg("slot");
    int slot; if(!numericArg("slot",0,MAX_STOPS-1,slot)||!slotEnabled(slot)){showMessage("Bitte einen aktiven Platz auswaehlen.");return;}
    if (stops[slot] != server.arg("expected")) { showMessage("Platz inzwischen geaendert. Suche erneut oeffnen."); return; }
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
    if(!numericArg("slot",0,MAX_STOPS-1,slot)||!slotEnabled(slot)||!slotEnabled(index)){showMessage("Ungueltige Position.");return;}

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
    html += "<section class='card'><a class='button full' href='/wifi'>WLAN-Netze</a><p><a class='button full' href='/display'>Helligkeit und Nachtmodus</a></p></section>";
    sendPage(html);
  });
  server.on("/device", HTTP_POST, []() {
    String name = server.arg("name"); name.trim();
    if (!name.length() || name.length() > 60) { showMessage("Bitte einen Namen mit höchstens 60 Zeichen eingeben."); return; }
    prefs.putString("boardname", name); redirectBoards();
  });
  server.on("/refresh", HTTP_POST, []() { lastRefresh = 0; redirectHome(); });

  // Neuer optionaler Kalibrierungs-Link; bestehende Web-App bleibt unveraendert.
  server.on("/touch-kalibrieren", HTTP_POST, []() {
    server.send(200, "text/plain; charset=utf-8", "Kalibrierung wird gestartet. Bitte die drei Kreuze am Display antippen.");
    Preferences tp; tp.begin("tramli-touch", false); tp.putBool("valid", false); tp.end();
    delay(100); ESP.restart();
  });
  setupExtendedRoutes();
  server.onNotFound([]() { showMessage("Diese Seite existiert nicht.", 404); });
  server.begin();
}
bool isLowBattery(float voltage, bool verified, float threshold) {
  return batteryEnabled && batteryWarningEnabled && verified && threshold > 0 && voltage > 0 && voltage < threshold;
}
int batteryBarWidth(float voltage) {
  if (voltage <= 0) return 0;
  return constrain((int)(voltage / 4.5f * 16 + 0.5f), 1, 16);
}
int batteryVisualColor(float voltage) {
  if (voltage <= 0) return 0x8410; // gray: invalid measurement
  if (isLowBattery(voltage, BATTERY_VERIFIED, batteryLowMv / 1000.0f)) return BUTTON_RED;
  if (batteryLowMv == 0 || voltage < batteryLowMv / 1000.0f + 0.20f) return AMBER;
  return 0x07E0; // green: >= configured threshold + 0.20 V, not a capacity estimate
}
float batteryVoltage() {
  if (!batteryEnabled) return -1.0f;
  static uint32_t sampledAt = 0;
  static bool sampled = false;
  static float voltage = -1.0f;
  if (sampled && millis() - sampledAt < 1000) return voltage;
  uint32_t total = 0;
  for (int i=0; i<16; ++i) total += analogReadMilliVolts(34);
  const float measured = (total / 16.0f) * BATTERY_DIVIDER / 1000.0f;
  // Plausibility bounds for the documented single-cell BAT+ input, not charge thresholds.
  voltage = measured >= 0.5f && measured <= 4.5f ? measured : -1.0f;
  sampledAt = millis(); sampled = true;
  return voltage;
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
  gfx->fillScreen(BLACK);
  drawLine("TOUCH KALIBRIEREN", 100, 8, AMBER, 2);
  drawLine("3 Kreuze nacheinander tippen", 105, 30, WHITE, 1);
  for(int i=0; i<3; ++i) {
    gfx->fillRect(0,48,480,272,BLACK);
    drawLine("Punkt " + String(i+1) + " von 3", 180, 152, WHITE, 2);
    gfx->drawCircle(tx[i], ty[i], 13, AMBER);
    gfx->drawFastHLine(tx[i]-18,ty[i],37, AMBER);
    gfx->drawFastVLine(tx[i],ty[i]-18,37, AMBER);
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
      gfx->fillScreen(BLACK);
      drawLine("Kalibrierung abgebrochen", 12, 80, AMBER, 2);
      delay(1200);
      return;
    }
  }
  const double a=calRawX[1]-calRawX[0], b=calRawX[2]-calRawX[0];
  const double c=calRawY[1]-calRawY[0], d=calRawY[2]-calRawY[0];
  const double det=a*d-b*c;
  if(det>-10000.0 && det<10000.0) {
    gfx->fillScreen(BLACK);
    drawLine("Kalibrierung ungueltig", 12, 80, AMBER, 2);
    delay(1500); return;
  }
  Preferences tp; tp.begin("tramli-touch", false);
  for(int i=0; i<3; ++i) {
    tp.putInt(("x"+String(i)).c_str(),calRawX[i]);
    tp.putInt(("y"+String(i)).c_str(),calRawY[i]);
  }
  tp.putBool("valid",true); tp.end();
  calValid=true;
  gfx->fillScreen(BLACK);
  drawLine("Kalibrierung gespeichert", 60, 120, AMBER, 2);
  delay(1200);
}


void touchButton(const String &label, int x, int y, int w, int h, uint16_t color = AMBER) {
  bool back = label == "ZURUECK";
  if (back && touchMode != 10) {
    gfx->fillRoundRect(x, y, w, h, 4, BUTTON_RED);
    color = WHITE;
  }
  if (touchMode == 10) {
    // Variant 4: charcoal fill, yellow labels and borders, including Back.
    gfx->fillRoundRect(x, y, w, h, 4, 0x10A2);
    color = 0xFF09;
  }
  gfx->drawRoundRect(x, y, w, h, 4, (back && touchMode != 10) ? AMBER : color);
  // Render ZURÜCK with the original bitmap font: U plus two explicit dots.
  // This avoids UTF-8 byte rendering and keeps the existing font unchanged.
  int textX = x + max(3, (w - (back ? 6 : (int)label.length()) * 12) / 2);
  int textY = y + (h - 16) / 2;
  drawLine(back ? String("ZURUCK") : label, textX, textY, color, 2);
  if (back) {
    gfx->fillRect(textX + 38, textY - 3, 2, 2, color);
    gfx->fillRect(textX + 42, textY - 3, 2, 2, color);
  }
}

// Two columns, seven rows. Gaps and the unused final cell are not selectable.
int cityAtTouch(int x, int y) {
  if (x < 12 || x >= 468 || y < 43 || y >= 302) return -1;
  int column = (x - 12) / 234;
  int row = (y - 43) / 37;
  if ((x - 12) % 234 >= 222 || (y - 43) % 37 >= 33) return -1;
  int city = row * 2 + column;
  return cityForPosition(city);
}
void drawCityMenu() {
  gfx->fillScreen(BLACK);
  drawLine("STADT AUSWAEHLEN", 12, 6, AMBER, 2);
  touchButton("ZURUECK", 352, 4, 116, 25, WHITE);
  gfx->drawFastHLine(10, 33, 460, AMBER);
  for (int position = 0; position < CITY_COUNT; ++position) {
    int city=cityForPosition(position);
    int x = 12 + (position % 2) * 234;
    int y = 43 + (position / 2) * 37;
    drawCityBadge(city,x,y,222,33,2,city==activeCity);
  }
}
void drawTouchMenu() {
  gfx->fillScreen(BLACK);
  drawLine("HALTESTELLE AENDERN", 12, 6, AMBER, 2);
  gfx->drawFastHLine(10, 30, 460, AMBER);
  if (touchEditIndex < 0 || touchEditIndex >= MAX_STOPS) return;
  String oldName = stops[touchEditIndex].length() ? stops[touchEditIndex] : "Freier Platz";
  drawLine(clip(oldName, 36), 12, 38, WHITE, 1);
  touchButton("ZURUECK", 352, 4, 116, 25, WHITE);
  if (touchMode == 3) {
    drawLine("Haltestelle entfernen?", 12, 88, WHITE, 2);
    drawLine("Andere Plaetze bleiben erhalten.", 12, 120, WHITE, 1);
    touchButton("JA, ENTFERNEN", 12, 170, 220, 45);
    touchButton("ZURUECK", 248, 170, 220, 45, WHITE);
  } else if (touchMode == 1) {
    gfx->fillRect(10, 57, 460, 26, BLACK);
    gfx->drawRect(10, 57, 460, 26, AMBER);
    drawLine(clip(touchQuery.length() ? touchQuery : "Suchbegriff eingeben", 36), 16, 62, WHITE, 2);
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
    if (stops[touchEditIndex].length()) touchButton("HALTESTELLE ENTFERNEN", 12, 290, 454, 28, WHITE);
  } else {
    drawLine("Suchresultate antippen:", 12, 65, WHITE, 2);
    if (!touchMatchCount) drawLine("Keine Treffer - erneut suchen", 12, 100, AMBER, 2);
    for (int i = 0; i < touchMatchCount; ++i) {
      int y = 91 + i * 37;
      touchButton(clip(touchMatches[i], 34), 12, y, 454, 34);
    }
    touchButton("ZURUECK", 12, 282, 454, 32, WHITE);
  }
}

void closeTouchMenu() {
  resetAutoPageTimer();
  touchMenuOpen = false;
  touchMode = 0;
  touchEditIndex = -1;
  ++touchExitRevision;
  drawBoard();
}

void touchSearch() {
  if (WiFi.status() != WL_CONNECTED) { drawLine("WLAN erforderlich", 12, 294, WHITE, 1); return; }
  touchQuery.trim();
  if (touchQuery.length() < 2) { drawLine("Mindestens 2 Zeichen", 12, 294, WHITE, 1); return; }
  touchButton("SUCHE...", 260, 246, 205, 40, WHITE);
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
  if (!touchMenuOpen) {
    if(homeCity<CITY_COUNT && x>=254 && x<314 && y>=270 && y<295){goHome();return;}
    // Direct page selection also works for empty pages.
    if (y >= 270 && y < 295 && x >= 320 && x < 470) {
      selectPage((x - 320) / 50);
      lastRefresh = 0;
      drawBoard();
      return;
    }
    if (x >= 12 && x < 112 && y >= 270 && y < 295) {
      touchMenuOpen = true; touchMode = 10; menuNotice = ""; drawExtendedMenu(); return;
    }
    // Nur der Haltestellentitel oeffnet das Bearbeitungsmenue.
    if (x > 8 && x < 470) {
      int position=-1;
      for(int p=0;p<pageSlots[activePage];++p){int hy=stationHeaderY(activePage,p);if(y>=hy-4&&y<hy+20)position=p;}
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
  if (extendedTap(x, y)) return;
  if (y < 32 && x > 345) { touchMode = 10; drawExtendedMenu(); return; }
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
            drawLine("Haltestelle bereits vorhanden", 12, 293, WHITE, 1);
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
  static bool pressed = false, waitRelease = false, moved = false, sliderDrag = false;
  static int firstX = 0, firstY = 0;
  static uint32_t started = 0;
  bool down = touch.touched();
  if(down) resetAutoPageTimer(); // Never advance beneath a finger or during a swipe.
  if (!down) {
    if(sliderDrag) { saveDisplaySettings(); sliderDrag=false; pressed=false; return; }
    if (pressed && !waitRelease && !moved && millis() - started < 1500) handleTouchTap(firstX, firstY);
    pressed = false; waitRelease = false; moved = false; return;
  }
  if (waitRelease) return;
  TS_Point p = touch.getPoint();
  int sx=touchXFromRaw(p.x,p.y), sy=touchYFromRaw(p.x,p.y);
  if(touchMenuOpen && touchMode==13 && pwmReady && sx>=20 && sx<=460 && ((sy>=67&&sy<87)||(sy>=126&&sy<146))) {
    int value=constrain((sx-20)*100/440,5,100);
    int &target=sy<100?brightness:nightBrightness;
    if(target!=value){target=value;applyBrightness();drawExtendedMenu();}
    sliderDrag=true;return;
  }
  if(sliderDrag) return;
  if (!pressed) { pressed = true; moved = false; firstX = p.x; firstY = p.y; started = millis(); return; }
  if (millis() - started > 1500) { waitRelease = true; return; }
  int dx = touchXFromRaw(p.x, p.y) - touchXFromRaw(firstX, firstY);
  int dy = touchYFromRaw(p.x, p.y) - touchYFromRaw(firstX, firstY);
  if (!touchMenuOpen && abs(dx) > 75 && abs(dx) > abs(dy)) {
    waitRelease = true;
    int next = constrain(activePage + (dx < 0 ? 1 : -1), 0, PAGE_COUNT - 1);
    if (next != activePage) { selectPage(next); drawBoard(); lastRefresh = 0; }
    return;
  }
  if (abs(dx) > 20 || abs(dy) > 20) moved = true; // kein Tippen bei Wischgeste
  // Ein Antippen wird erst beim Loslassen ausgeloest: Wischen bleibt priorisiert.
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
analogSetPinAttenuation(34, ADC_11db);
  pinMode(27, OUTPUT); digitalWrite(27, HIGH);
  pinMode(33, OUTPUT); digitalWrite(33, HIGH); // Touch-CS inaktiv
  pinMode(5, OUTPUT); digitalWrite(5, HIGH);   // SD-CS inaktiv
  gfx->begin(27000000);
  setupTouch();
  loadTouchCalibration();
  gfx->setTextWrap(false);
  gfx->fillScreen(BLACK);
  loadSettings();
  WiFi.persistent(false);
  WiFi.setAutoReconnect(false);
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("TRAMLI-BERN-2");
  accessPoint = true;
  configTzTime("CET-1CEST,M3.5.0,M10.5.0/3", "pool.ntp.org");
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  pwmReady = ledcAttach(27, 5000, 8);
#else
  pwmReady = ledcSetup(0, 5000, 8) > 0;
  if (pwmReady) ledcAttachPin(27, 0);
#endif
  applyBrightness();
  // Restore the saved city before starting its intro; no internet dependency.
  if(introMode!=0) bootIntro.begin(gfx, millis(), activeCity);
  while (bootIntro.active()) {
    bootIntro.tick(gfx, millis());
    delay(1); // Yield to ESP32 background tasks and watchdog.
  }
  setupRoutes();
  calibrateTouch();
  drawBoard();
  resetAutoPageTimer();
}

void loop() {
  server.handleClient();
  serviceWifiScan();
  serviceWifi();
  serviceDepartures();
  applyBrightness();
  if (bootIntro.active()) {
    resetAutoPageTimer();
    if (bootIntro.tick(gfx, millis())) {
      ++touchExitRevision; // Force full redraw even after rapid A -> B -> A changes.
      drawBoard();
      Serial.printf("[Intro] city=%d freeHeap=%u minHeap=%u\n", activeCity,
                    (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getMinFreeHeap());
    }
    delay(5);
    return; // Network/web work above continues; touch and board drawing wait.
  }
  checkSwipe();
  serviceAutoPages();
  static bool wasConnected = false;
  bool connected = WiFi.status() == WL_CONNECTED;
  if (connected != wasConnected) {
    wasConnected = connected;
    lastRefresh = 0;
    if (!connected) {
      departureCount = 0;
      WiFi.mode(WIFI_AP_STA);
      WiFi.softAP("TRAMLI-BERN-2");
    }
    if (!touchMenuOpen) drawBoard();
  }
  if (!touchMenuOpen && connected && (lastRefresh == 0 || millis() - lastRefresh >= REFRESH_MS)) {
    refreshBoard();
    lastRefresh = millis();
  }
  if (!touchMenuOpen) retryFailedStops();
  static uint32_t lastBlink = 0;
  if (millis() - lastBlink >= 500) {
    lastBlink = millis();
    if (!touchMenuOpen) { drawHeader(); if (connected && stopCount) drawDepartureRows(); }
  }
  static uint32_t lastWarning = 0;
  if (millis() - lastWarning >= 220) {
    lastWarning = millis();
  }
  delay(5);

}


// ---- Extended controls. Legacy NVS namespace and slot keys remain unchanged. ----
String wifiKey(int i) { return "wifi" + String(i); }
void storeNetwork(int i) { prefs.putBytes(wifiKey(i).c_str(), &networks[i], sizeof(WifiProfile)); }
int networkCount() { int n=0; for (auto &v:networks) if(v.ssid[0]) ++n; return n; }
void loadExtendedSettings() {
  // RC3 explicitly enables the manual indicator once, including upgrades from RC2.
  // Later user choices remain persistent and are never reset on ordinary reboot.
  if (!prefs.getBool("batteryRC3", false)) {
    prefs.putBool("batteryOn", true);
    prefs.putBool("batteryRC3", true);
  }
  batteryEnabled = prefs.getBool("batteryOn", true);
  batteryWarningEnabled = prefs.getBool("batteryWarn", false);
  batteryLowMv = prefs.getInt("batteryLowMv", 0);
  if (batteryLowMv < 2500 || batteryLowMv > 4300 || batteryLowMv % 50) batteryLowMv = 0;
  if (!prefs.getBool("wifiV2", false)) {
    if (wifiSsid.length() && wifiSsid.length() <= 32 && wifiPassword.length() <= 63) {
      strlcpy(networks[0].ssid, wifiSsid.c_str(), sizeof(networks[0].ssid));
      strlcpy(networks[0].pass, wifiPassword.c_str(), sizeof(networks[0].pass));
      networks[0].worked = 1; storeNetwork(0);
    }
    prefs.putBool("wifiV2", true);
  }
  for (int i=0;i<4;++i) {
    if (prefs.getBytesLength(wifiKey(i).c_str()) == sizeof(WifiProfile))
      prefs.getBytes(wifiKey(i).c_str(), &networks[i], sizeof(WifiProfile));
    networks[i].ssid[32]=0; networks[i].pass[63]=0;
  }
  wifiPreferred = constrain((int)prefs.getUChar("wifiPref",0),0,3);
  brightness = constrain(prefs.getInt("bright",100),5,100);
  nightBrightness = constrain(prefs.getInt("nightBright",15),5,100);
  nightStart = constrain(prefs.getInt("nightStart",1320),0,1439);
  nightEnd = constrain(prefs.getInt("nightEnd",420),0,1439);
  nightEnabled = prefs.getBool("nightOn",false);
  wifiPassword = ""; // Loaded passwords are never rendered or logged.
}
bool validNetwork(const String &ssid, const String &pass) {
  return ssid.length()>0 && ssid.length()<=32 && (pass.length()==0 || (pass.length()>=8 && pass.length()<=63));
}
int saveNetwork(const String &ssid, const String &pass) {
  if (!validNetwork(ssid,pass)) return -1;
  int slot=-1;
  for(int i=0;i<4;++i) if(String(networks[i].ssid)==ssid) { slot=i; break; }
  if(slot<0) for(int i=0;i<4;++i) if(!networks[i].ssid[0]) { slot=i; break; }
  if(slot<0) return -1;
  // A proven credential is never overwritten by an untested password.
  if(networks[slot].worked && String(networks[slot].pass)!=pass) return -1;
  bool known=networks[slot].worked;
  memset(&networks[slot],0,sizeof(WifiProfile));
  strlcpy(networks[slot].ssid,ssid.c_str(),sizeof(networks[slot].ssid));
  strlcpy(networks[slot].pass,pass.c_str(),sizeof(networks[slot].pass));
  networks[slot].worked=known; storeNetwork(slot);
  return slot;
}
bool deleteNetwork(int i) {
  if(i<0||i>=4||!networks[i].ssid[0]||networkCount()<=1) return false;
  int proven=0; for(auto &v:networks) if(v.ssid[0]&&v.worked) ++proven;
  if(networks[i].worked && proven<=1) return false;
  bool active=WiFi.status()==WL_CONNECTED && WiFi.SSID()==networks[i].ssid;
  memset(&networks[i],0,sizeof(WifiProfile)); storeNetwork(i);
  if(active||wifiTrying==i) { WiFi.disconnect(); wifiAttempting=false; wifiTrying=-1; }
  return true;
}
void chooseNetwork(int i) {
  if(i<0||i>=4||!networks[i].ssid[0]) return;
  wifiPreferred=i; prefs.putUChar("wifiPref",i);
  WiFi.disconnect(); wifiAttempting=false; wifiTrying=-1;
}
void serviceWifi() {
  if(wifiScanning) return;
  if(WiFi.status()==WL_CONNECTED) {
    if(wifiTrying>=0 && !networks[wifiTrying].worked && WiFi.SSID()==networks[wifiTrying].ssid) {
      networks[wifiTrying].worked=1; storeNetwork(wifiTrying);
    }
    wifiAttempting=false;
    if(accessPoint) { WiFi.softAPdisconnect(true); accessPoint=false; }
    return;
  }
  if(!accessPoint) { WiFi.mode(WIFI_AP_STA); WiFi.softAP("TRAMLI-BERN-2"); accessPoint=true; }
  if(wifiAttempting && millis()-wifiAttemptAt<15000) return;
  int next=wifiTrying<0 ? wifiPreferred : (wifiTrying+1)%4;
  for(int n=0;n<4;++n,next=(next+1)%4) if(networks[next].ssid[0]) {
    WiFi.disconnect(); wifiTrying=next; wifiAttemptAt=millis(); wifiAttempting=true;
    WiFi.begin(networks[next].ssid,networks[next].pass); return;
  }
}
// Asynchronous discovery; bounded cache, no password storage and no automatic join.
void startWifiScan(bool force) {
  if(wifiScanning || (!force && wifiScanDone && millis()-wifiScanAt<60000)) return;
  if(WiFi.status()!=WL_CONNECTED) {
    WiFi.disconnect(); wifiAttempting=false; wifiTrying=-1;
    if(!accessPoint) { WiFi.mode(WIFI_AP_STA); WiFi.softAP("TRAMLI-BERN-2"); accessPoint=true; }
  }
  WiFi.scanDelete();
  nearbyCount=0; nearbyPage=0; wifiScanError=false; wifiScanDone=false;
  wifiScanAt=millis(); wifiScanning=true;
  int result=WiFi.scanNetworks(true,false);
  if(result==WIFI_SCAN_FAILED) {wifiScanning=false;wifiScanDone=true;wifiScanError=true;}
}
void serviceWifiScan() {
  if(!wifiScanning) return;
  int n=WiFi.scanComplete();
  if(n==WIFI_SCAN_RUNNING && millis()-wifiScanAt<30000) return;
  if(n<0) {esp_wifi_scan_stop();wifiScanError=true;}
  else for(int i=0;i<n;++i) {
    String ssid=WiFi.SSID(i); if(!ssid.length() || ssid.length()>32) continue;
    int rssi=WiFi.RSSI(i), found=-1;
    bool open=WiFi.encryptionType(i)==WIFI_AUTH_OPEN;
    for(int j=0;j<nearbyCount;++j) if(nearbyWifi[j].ssid==ssid && nearbyWifi[j].open==open) {found=j;break;}
    if(found>=0) {if(rssi>nearbyWifi[found].rssi) nearbyWifi[found].rssi=rssi;}
    else {
      int slot=nearbyCount;
      if(slot>=16) {slot=15;if(rssi<=nearbyWifi[slot].rssi) continue;}
      else ++nearbyCount;
      nearbyWifi[slot]={ssid,rssi,open};
    }
    for(int a=0;a<nearbyCount;++a)for(int b=a+1;b<nearbyCount;++b)
      if(nearbyWifi[b].rssi>nearbyWifi[a].rssi) {NearbyWifi temp=nearbyWifi[a];nearbyWifi[a]=nearbyWifi[b];nearbyWifi[b]=temp;}
  }
  WiFi.scanDelete(); wifiScanning=false; wifiScanDone=true;wifiScanAt=millis();nearbyPage=0;
  if(touchMenuOpen && touchMode==24) drawExtendedMenu();
}
String wifiScanJson() {
  JsonDocument d; d["busy"]=wifiScanning;d["error"]=wifiScanError;
  JsonArray rows=d["networks"].to<JsonArray>();
  for(int i=0;i<nearbyCount;++i) {JsonObject row=rows.add<JsonObject>();row["ssid"]=nearbyWifi[i].ssid;row["rssi"]=nearbyWifi[i].rssi;row["open"]=nearbyWifi[i].open;}
  String out;serializeJson(d,out);return out;
}
bool nightAt(int minute) {
  if(nightStart==nightEnd) return false;
  return nightStart<nightEnd ? minute>=nightStart && minute<nightEnd : minute>=nightStart || minute<nightEnd;
}
void applyBrightness() {
  if(!pwmReady) return;
  int value=brightness;
  time_t now=time(nullptr);
  if(nightEnabled && now>1700000000) {
    struct tm t; localtime_r(&now,&t);
    if(nightAt(t.tm_hour*60+t.tm_min)) value=min(brightness,nightBrightness);
  }
  static int previous=-1;
  if(value==previous) return;
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  if(!ledcWrite(27,value*255/100)) { pwmReady=false; return; }
#else
  ledcWrite(0,value*255/100);
#endif
  previous=value;
}
void saveDisplaySettings() {
  prefs.putInt("bright",brightness); prefs.putInt("nightBright",nightBrightness);
  prefs.putBool("nightOn",nightEnabled); prefs.putInt("nightStart",nightStart); prefs.putInt("nightEnd",nightEnd);
  applyBrightness();
}
String clockMinutes(int value) { value=constrain(value,0,1439); char b[6]; snprintf(b,sizeof(b),"%02d:%02d",value/60,value%60); return String(b); }
int readClock(const String &s) {
  if(s.length()!=5||s[2]!=':') return -1;
  for(int i: {0,1,3,4}) if(s[i]<'0'||s[i]>'9') return -1;
  int h=s.substring(0,2).toInt(),m=s.substring(3).toInt();
  return h<24 && m<60 ? h*60+m : -1;
}
bool numericArg(const char *name,int low,int high,int &out) {
  String v=server.arg(name); if(!v.length()||v.length()>5) return false;
  for(size_t i=0;i<v.length();++i) if(v[i]<'0'||v[i]>'9') return false;
  out=v.toInt(); return out>=low&&out<=high;
}
String stopHealth(int i) {
  if(!stops[i].length()) return "Freier Platz";
  if(WiFi.status()!=WL_CONNECTED) return "WLAN getrennt - Wiederverbindung automatisch";
  if(stopFetchFailed[i]) return "Server/Verbindung voruebergehend nicht verfuegbar - Wiederholung automatisch";
  if(!lastSuccess[i]) return "Noch nicht abgerufen - Seite auswaehlen";
  if(millis()-lastSuccess[i]>90000) return "Gespeicherte Antwort ist veraltet";
  return "Antwort empfangen; Echtzeitstatus des Betreibers nicht bestaetigt";
}
String diagnosticsHtml() {
  String h="<section class='card'><h2>Verbindung und Gerät</h2><p>";
  h+=WiFi.status()==WL_CONNECTED ? "WLAN verbunden: "+escapeHtml(WiFi.SSID()) : "WLAN getrennt; gespeicherte Netze werden automatisch versucht";
  h+="</p><p>IP: "+escapeHtml(currentAddress())+"</p><p>RAM frei: "+String(ESP.getFreeHeap())+" Byte; Minimum: "+String(ESP.getMinFreeHeap())+" Byte</p><p>"+String(VERSION);
  h += "</p><p>Intro-Grafik: " + String(IntroAssets::assetBytes + IntroCrests::assetBytes + MenuCrests::assetBytes) + " Byte Flash; Renderer: " + String(sizeof(BootIntro)) + " Byte RAM";
  h += "</p><p>Programm: " + String(ESP.getSketchSize()) + " Byte</p><p>Batterie: ";
  float v=batteryVoltage();
  if(!batteryEnabled) h+="Anzeige ausgeschaltet (Menü → Baterie).";
  else if(v<0) h+="kein plausibler BAT+ Messwert / eventuell nicht angeschlossen. Keine Prozentangabe.";
  else h+=String(v,2)+" V"+(isLowBattery(v,BATTERY_VERIFIED,(batteryLowMv / 1000.0f))?" - WARNUNG: Batteriespannung niedrig":"");
  h+="</p><p>Batteriewarnung: "+String(!batteryWarningEnabled?"aus":!batteryEnabled?"wartet: Batterieanzeige ausgeschaltet":(!BATTERY_VERIFIED || (batteryLowMv / 1000.0f)<=0)?"vorbereitet; Spannungsschwelle noch nicht festgelegt":"eingeschaltet");
  h+="</p><p>Helligkeitssteuerung: "+String(pwmReady?"PWM bereit":"nicht verfuegbar")+"</p><p>Uhr: "+String(time(nullptr)>1700000000?"synchronisiert":"wartet auf NTP")+"</p></section>";
  for(int i=0;i<MAX_STOPS;++i) {
    h+="<section class='card'><h2>Seite "+String(i/STOPS_PER_PAGE+1)+", Platz "+String(i%STOPS_PER_PAGE+1)+"</h2><p>"+escapeHtml(stops[i])+"</p><p>"+escapeHtml(stopHealth(i))+"</p>";
    if(lastSuccess[i]) {
      uint32_t age=millis()-lastSuccess[i];
      h+="<p>Letzter erfolgreicher Empfang vor "+String(age/1000)+" s.</p>";
      if(time(nullptr)>1700000000) {
        time_t stamp=time(nullptr)-age/1000; struct tm t;localtime_r(&stamp,&t);char b[24];strftime(b,sizeof(b),"%d.%m.%Y %H:%M:%S",&t);
        h+="<p>Empfang: "+String(b)+" (aus Laufzeit abgeleitet).</p>";
      }
    }
    if(stopFetchFailed[i]) h+="<p>Naechster Versuch auf aktiver Seite in hoechstens "+String(retryDelay[i]/1000)+" s nach Ende des letzten Versuchs.</p>";
    h+="</section>";
  }
  h+="<p class='hint'>Empfangszeit ist nicht der Aktualisierungszeitpunkt des Verkehrsbetreibers. Inaktive Seiten werden bei Auswahl geladen. Alte Abfahrten werden nach 3 Minuten ausgeblendet. Technische Verbindungsfehler sind keine Verkehrsmeldungen.</p>";
  return h;
}
void setBatteryEnabled(bool enabled) {
  batteryEnabled = enabled;
  prefs.putBool("batteryOn", enabled);
  ++screenRevision;
  if (!touchMenuOpen) drawHeader();
}
void setBatteryWarningEnabled(bool enabled) {
  batteryWarningEnabled = enabled;
  prefs.putBool("batteryWarn", enabled);
  ++screenRevision;
  if (!touchMenuOpen) drawHeader();
}
void setBatteryThreshold(int mv) {
  if (mv != 0 && (mv < 2500 || mv > 4300 || mv % 50)) return;
  batteryLowMv = mv;
  prefs.putInt("batteryLowMv", mv);
  ++screenRevision;
  if (!touchMenuOpen) drawHeader();
}
void setupExtendedRoutes() {
  server.on("/battery",HTTP_GET,[](){
    String h=pageStart("Baterie")+"<section class='card'><h2>Batterieanzeige</h2><p>"+String(batteryEnabled?"Eingeschaltet":"Ausgeschaltet")+"</p><form method='post' action='/battery'><label><input type='checkbox' name='enabled' value='yes'"+String(batteryEnabled?" checked":"")+"> Batterie anzeigen</label><label><input type='checkbox' name='warn' value='yes'"+String(batteryWarningEnabled?" checked":"")+"> Warnung bei niedriger Spannung</label><p class='hint'>Warnschwelle passend zur eingesetzten Batterie waehlen. Unterhalb: rotes Symbol und Diagnosemeldung. Keine automatische Abschaltung.</p></form><p class='hint'>Gespeichert auch nach Neustart. 16 ADC-Messungen, Teiler R2/R3: 100k/100k. Anzeige in Volt, keine geschaetzten Prozentwerte. Ohne plausiblen Wert: --. Mit USB kann BAT+ auch ohne Batterie Spannung fuehren.</p></section>";
    String options="<label>Warnschwelle (Volt)<select name='threshold'><option value='0'>Nicht eingestellt</option>";
    for(int mv=2500;mv<=4300;mv+=50) options+="<option value='"+String(mv)+"'"+String(mv==batteryLowMv?" selected":"")+">"+String(mv/1000.0f,2)+" V</option>";
    options+="</select></label><button>Speichern</button></form>";
    h.replace("</form>",options);
    sendPage(h);
  });
  server.on("/battery",HTTP_POST,[](){
    if(server.hasArg("enabled") && server.arg("enabled")!="yes") {showMessage("Ungueltiger Batteriestatus");return;}
    if(server.hasArg("warn") && server.arg("warn")!="yes") {showMessage("Ungueltige Warnungseinstellung");return;}
    int threshold=batteryLowMv;
    if(server.hasArg("threshold") && (!numericArg("threshold",0,4300,threshold) || (threshold && (threshold<2500 || threshold%50)))) {showMessage("Ungueltige Warnschwelle");return;}
    setBatteryThreshold(threshold);
    setBatteryEnabled(server.arg("enabled")=="yes");
    setBatteryWarningEnabled(server.arg("warn")=="yes");
    server.sendHeader("Location","/battery");server.send(303);
  });
  server.on("/",HTTP_GET,[](){
    String h=pageStart("Menü");
    const char *paths[]={"/wifi","/stops","/cities","/display","/diagnostics","/system","/boards","/battery"};
    const char *labels[]={"◉ Wi-Fi-Netze","▤ Seiten und Haltestellen","↔ Stadt auswählen","☀ Helligkeit / Nachtmodus","i · Diagnose","⚙ System","▣ Meine Boards","▣ Baterie"};
    for(int i=0;i<8;++i) h+="<section class='card'><a class='button full' href='"+String(paths[i])+"'>"+labels[i]+"</a></section>";
    h+="<section class='card'><a class='button full' href='/comfort'>Favoriten / Intro / Seiten / Home</a></section>";
    if(homeCity<CITY_COUNT)h+="<form method='post' action='/home'><button>Home</button></form>";
    sendPage(h);
  });
  setupComfortRoutes();
  server.on("/cities",HTTP_GET,[](){
    String h=pageStart("Stadt auswählen")+"<section class='card'><form method='post' action='/city'><label>Stadt</label><select name='city'>";
    for(int position=0;position<CITY_COUNT;++position){int i=cityForPosition(position);h+="<option value='"+String(i)+"'"+(i==activeCity?" selected":"")+">"+escapeHtml(CITY_QUERIES[i])+"</option>";}
    sendPage(h+"</select><button>Auswählen</button></form></section>");
  });
  server.on("/wifi/scan",HTTP_GET,[](){server.sendHeader("Cache-Control","no-store");server.send(200,"application/json",wifiScanJson());});
  server.on("/wifi/scan",HTTP_POST,[](){startWifiScan(true);server.send(202,"application/json",wifiScanJson());});
  server.on("/wifi",HTTP_GET,[](){
    startWifiScan();
    String h=pageStart("Wi-Fi-Netze")+"<p class='hint'>Vier Speicherplätze. 2,4-GHz-WLAN. Die letzte gespeicherte und die letzte erfolgreich verwendete Verbindung bleiben geschützt. Nach einem Netzwechsel kann sich die IP ändern.</p>";
    h+=R"SCAN(<section class='card'><h2>Verfügbare Netze</h2><p id='scanStatus' role='status'>Suche läuft…</p><div id='scanRows'></div><button type='button' id='scanAgain'>Erneut suchen</button><p class='hint'>Bis zu 16 Netze, stärkstes Signal zuerst. Auswahl übernimmt den Namen unten. Versteckte Netze manuell eingeben.</p></section>
<script>
(function(){let timer;const status=document.getElementById('scanStatus'),rows=document.getElementById('scanRows'),again=document.getElementById('scanAgain');
async function poll(restart){clearTimeout(timer);again.disabled=true;try{const r=await fetch('/wifi/scan',{method:restart?'POST':'GET',cache:'no-store'});if(!r.ok)throw Error();const d=await r.json();rows.textContent='';status.textContent=d.busy?'Suche läuft…':d.error?'Suche fehlgeschlagen. Erneut versuchen.':d.networks.length?'Netz auswählen:':'Keine sichtbaren Netze gefunden.';
if(!d.busy)d.networks.forEach(n=>{const b=document.createElement('button');b.type='button';b.className='full';b.style.marginBottom='8px';b.textContent=n.ssid+' · '+n.rssi+' dBm · '+(n.open?'Offen':'Gesichert');b.onclick=()=>{const f=document.querySelector('input[name="ssid"]');f.value=n.ssid;document.querySelector('input[name="pass"]').value='';f.scrollIntoView({block:'center'});document.querySelector('input[name="pass"]').focus();};rows.appendChild(b);});
if(d.busy)timer=setTimeout(()=>poll(false),1200);else again.disabled=false;
}catch(e){status.textContent='Keine Antwort. Verbindung prüfen und erneut suchen.';again.disabled=false;}}
again.onclick=()=>poll(true);poll(false);})();
</script>)SCAN";
    for(int i=0;i<4;++i) if(networks[i].ssid[0]) {
      h+="<section class='card'><h2>"+escapeHtml(networks[i].ssid)+"</h2><p>"+String(networks[i].worked?"Bereits verbunden":"Noch nicht erfolgreich verbunden")+"</p>";
      h+="<form action='/wifi/select' method='post'><input type='hidden' name='index' value='"+String(i)+"'><button>Verbinden</button></form>";
      h+="<form action='/wifi/delete' method='post'><input type='hidden' name='index' value='"+String(i)+"'><label><input type='checkbox' name='confirm' value='yes' required> Dieses Netz entfernen</label><button class='subtle'>Entfernen bestätigen</button></form></section>";
    }
    h+="<section class='card'><h2>Netz hinzufügen</h2><form method='post' action='/wifi'><label>SSID</label><input name='ssid' maxlength='32' autocapitalize='none' spellcheck='false' required><label>Passwort</label><input name='pass' type='password' maxlength='63' autocomplete='new-password'><p class='hint'>8–63 Zeichen, oder leer für ein offenes Netz. Ein bewährtes Passwort wird nicht ungetestet überschrieben.</p><button>Speichern und verbinden</button></form></section>";
    sendPage(h);
  });
  server.on("/wifi",HTTP_POST,[](){
    int i=saveNetwork(server.arg("ssid"),server.arg("pass"));
    if(i<0){showMessage("SSID/Passwort ungueltig, Speicher voll oder bestehendes Passwort geschuetzt.");return;}
    sendPage(pageStart("WLAN gespeichert")+"<p>Verbindung wird versucht. Falls kein gespeichertes Netz erreichbar ist: TRAMLI-BERN-2 / 192.168.4.1.</p>");
    chooseNetwork(i);
  });
  server.on("/wifi/select",HTTP_POST,[](){int i;if(!readIndex(4,i)||!networks[i].ssid[0]){showMessage("Netz ungueltig");return;}sendPage(pageStart("Netzwechsel")+"<p>Neue IP am Display ablesen.</p>");chooseNetwork(i);});
  server.on("/wifi/delete",HTTP_POST,[](){int i;if(server.arg("confirm")!="yes"||!readIndex(4,i)||!deleteNetwork(i)){showMessage("Nicht entfernt: letzte gespeicherte oder letzte bewaehrte Verbindung ist geschuetzt.");return;}server.sendHeader("Location","/wifi");server.send(303);});
  server.on("/diagnostics",HTTP_GET,[](){sendPage(pageStart("Diagnose")+diagnosticsHtml());});
  server.on("/display",HTTP_GET,[](){
    String h=pageStart("Display")+"<section class='card'>";
    if(!pwmReady) {sendPage(h+"<p>PWM konnte nicht initialisiert werden. Helligkeit nicht steuerbar.</p></section>");return;}
    h+="<form method='post' action='/display'><label>Manueller Tageswert: "+String(brightness)+" %</label><input aria-label='Helligkeit' name='brightness' type='range' oninput='this.previousElementSibling.textContent = this.value + String.fromCharCode(37)' min='5' max='100' value='"+String(brightness)+"'><label>Nachtwert: "+String(nightBrightness)+" %</label><input aria-label='Nachthelligkeit' name='nightBrightness' type='range' oninput='this.previousElementSibling.textContent = this.value + String.fromCharCode(37)' min='5' max='100' value='"+String(nightBrightness)+"'><label><input type='checkbox' name='enabled' value='yes'"+String(nightEnabled?" checked":"")+"> Nachtmodus</label><label>Beginn</label><input name='start' type='time' value='"+clockMinutes(nightStart)+"' required><label>Ende</label><input name='end' type='time' value='"+clockMinutes(nightEnd)+"' required><button>Speichern</button></form><p class='hint'>Ortszeit Schweiz. Ohne synchronisierte Uhr bleibt der Tageswert aktiv. Gleiche Start- und Endzeit deaktiviert das Zeitfenster. Nachts wird der Tageswert nicht erhöht.</p></section>";
    sendPage(h);
  });
  server.on("/display",HTTP_POST,[](){
    int b,n,start=readClock(server.arg("start")),end=readClock(server.arg("end"));
    if(!pwmReady||!numericArg("brightness",5,100,b)||!numericArg("nightBrightness",5,100,n)||start<0||end<0){showMessage("Ungueltige Displayeinstellungen");return;}
    brightness=b;nightBrightness=n;nightStart=start;nightEnd=end;nightEnabled=server.arg("enabled")=="yes";saveDisplaySettings();
    server.sendHeader("Location","/display");server.send(303);
  });
  server.on("/system",HTTP_GET,[](){
    sendPage(pageStart("System")+"<section class='card'><h2>"+String(VERSION)+"</h2><p>Private Abfahrtstafel. Daten: <a href='https://transport.opendata.ch/'>transport.opendata.ch</a>. Kein Verkehrsmeldungsdienst.</p><form action='/refresh' method='post'><button>Daten aktualisieren</button></form><p><a class='button' href='/settings'>Boardname / Einstellungen</a></p><form action='/touch-kalibrieren' method='post'><button class='subtle'>Touch neu kalibrieren (Neustart)</button></form></section>");
  });
}

void menuTop(const String &title) {
  gfx->fillScreen(BLACK); drawLine(title,12,8,touchMode==10?0xFF09:AMBER,2);
  touchButton("ZURUECK",352,4,116,27,BUTTON_BLUE);gfx->drawFastHLine(10,36,460,touchMode==10?0xFF09:AMBER);
}
void sliderDraw(int y,int value,const String &label) {
  drawLine(label+" "+String(value)+"%",12,y,AMBER,2);
  gfx->drawRoundRect(20,y+22,440,20,6,AMBER);
  gfx->fillRoundRect(20,y+22,max(8,value*440/100),20,6,AMBER);
}
const char *keyboardRow(int r) {
  static const char *keys[3][4]={
    {"qwertzuiop","asdfghjkl","yxcvbnm","0123456789"},
    {"QWERTZUIOP","ASDFGHJKL","YXCVBNM","0123456789"},
    {"!@#$%^&*()","-_=+[]{}\\|",";:'\",.<>/?","`~"}
  };
  return keys[keyboardPage][r];
}
void drawExtendedMenu() {
  if(touchMode==25) {
    menuTop("KOMFORT");
    touchButton("FAVORITEN",12,46,456,40);
    touchButton(introMode==2?"INTRO: START + STADT":introMode==1?"INTRO: NUR START":"INTRO: AUS",12,94,456,40);
    touchButton("-",12,142,66,40);
    touchButton(autoPageSeconds?"AUTO-SEITEN: "+String(autoPageSeconds)+" S":"AUTO-SEITEN: AUS",86,142,308,40);
    touchButton("+",402,142,66,40);
    touchButton("AKTUELLE SEITE ALS HOME",12,190,456,40);
    touchButton(homeCity<CITY_COUNT?"HOME: "+String(CITY_LABELS[homeCity])+" / "+String(homePage+1):"HOME: NICHT GESETZT",12,238,456,40);
    drawLine("Antippen aendert und speichert die Einstellung.",12,295,WHITE,1);
  } else if(touchMode==26) {
    menuTop("FAVORITEN");
    // Keep the editor in canonical order so toggling does not move the target.
    for(int city=0;city<CITY_COUNT;++city)
      drawCityBadge(city,12+(city%2)*234,43+(city/2)*37,222,33,2,(favoriteCities>>city)&1);
  } else if(touchMode==10) {
    menuTop("MENU");
    const char *labels[]={"WI-FI NETZE","EINSTELLUNG","SEITEN","STADT","DIAGNOSE","AKTUALISIEREN","UEBER / SYSTEM","HALTESTELLEN"};
    for(int i=0;i<8;++i)touchButton(labels[i],12+(i%2)*234,46+(i/2)*57,222,49);
    touchButton("BATERIE",12,278,456,34);
  } else if(touchMode==11) {
    menuTop("SEITEN / PLATZE");
    for(int i=0;i<3;++i)touchButton("SEITE "+String(i+1),12+i*156,46,144,38,i==activePage?BUTTON_BLUE:AMBER);
    for(int pos=0;pos<pageSlots[activePage];++pos){int i=activePage*STOPS_PER_PAGE+pos;touchButton(String(activePage+1)+"."+String(pos+1)+" "+clip(stops[i].length()?stops[i]:"Frei",28),12,94+pos*40,456,36);}
    touchButton("PLATZE: "+String(pageSlots[activePage])+"  (1-3)",12,270,456,38);
    if(menuNotice.length())drawLine(clip(menuNotice,70),12,257,AMBER,1);
  } else if(touchMode==12) {
    menuTop("WI-FI NETZE");
    for(int i=0;i<4;++i)touchButton(String(i+1)+" "+clip(networks[i].ssid[0]?String(networks[i].ssid):"Freier Platz",29),12,46+i*45,456,40);
    touchButton("NETZE SUCHEN",12,232,222,44);
    touchButton("MANUELL",246,232,222,44);
    drawLine(clip(menuNotice,70),12,290,WHITE,1);
  } else if(touchMode==24) {
    menuTop("NETZE SUCHEN");
    if(wifiScanning) drawLine("Suche laeuft...",12,55,AMBER,2);
    else if(wifiScanError) drawLine("Suche fehlgeschlagen",12,55,AMBER,2);
    else if(!nearbyCount) drawLine("Keine Netze gefunden",12,55,WHITE,2);
    else for(int r=0;r<4;++r) {int i=nearbyPage*4+r;if(i>=nearbyCount)break;
      touchButton(clip(nearbyWifi[i].ssid,22)+" "+String(nearbyWifi[i].rssi)+(nearbyWifi[i].open?" O":" S"),12,46+r*48,456,44);
    }
    touchButton("<",12,243,66,44);touchButton("NEU SUCHEN",86,243,220,44);touchButton(">",314,243,66,44);touchButton("NAME",388,243,80,44);
    drawLine("O=offen S=gesichert | "+String(nearbyPage+1)+"/"+String(max(1,(nearbyCount+3)/4)),12,300,WHITE,1);
  } else if(touchMode==13) {
    menuTop("DISPLAY");
    touchButton("FAVORITEN / INTRO / HOME",12,276,456,36);
    if(!pwmReady){drawLine("PWM nicht verfuegbar",12,70,WHITE,2);return;}
    sliderDraw(45,brightness,"HELLIGKEIT");sliderDraw(104,nightBrightness,"NACHTWERT");
    touchButton(nightEnabled?"NACHT EIN":"NACHT AUS",12,163,222,36);
    touchButton("ZEITEN",246,163,222,36);
    drawLine(clockMinutes(nightStart)+" - "+clockMinutes(nightEnd),12,217,WHITE,2);
    drawLine("Speichert automatisch. Ohne Uhr: Tageswert.",12,253,WHITE,1);
  } else if(touchMode==14) {
    menuTop("DIAGNOSE");
    drawLine(WiFi.status()==WL_CONNECTED?"WLAN: "+clip(WiFi.SSID(),26):"WLAN getrennt; Auto-Versuch",12,48,WHITE,1);
    drawLine("IP "+currentAddress()+"  RAM "+String(ESP.getFreeHeap()),12,68,WHITE,1);
    for(int pos=0;pos<pageSlots[activePage];++pos){int i=activePage*STOPS_PER_PAGE+pos;drawLine(String(activePage+1)+"."+String(pos+1)+": "+clip(stops[i],36),12,96+pos*40,AMBER,1);drawLine(clip(stopHealth(i),70),12,108+pos*40,WHITE,1);}
    drawLine(batteryEnabled?"BAT+: "+(batteryVoltage()>0?String(batteryVoltage(),2)+" V":"--")+" (Details im Web)":"Batterieanzeige ausgeschaltet.",12,270,WHITE,1);
    touchButton("NEU LADEN",12,287,222,29);
  } else if(touchMode==15) {
    menuTop("UEBER / SYSTEM");
    drawLine(VERSION,12,58,AMBER,2);drawLine("Daten: transport.opendata.ch",12,97,WHITE,2);
    drawLine("Keine Verkehrsmeldungen",12,137,WHITE,2);
    drawLine("BAT+ Messung: GPIO34, Teiler 1:2",12,177,WHITE,1);
    drawLine("Kalibrierung / Boardname in der Web-App",12,202,WHITE,1);
    touchButton("DATEN AKTUALISIEREN",12,246,456,45);
  } else if(touchMode==16) {
    menuTop("HALTESTELLE");
    drawLine(clip(stops[touchEditIndex].length()?stops[touchEditIndex]:"Freier Platz",37),12,51,WHITE,2);
    touchButton("SUCHEN / AENDERN",12,94,456,45);touchButton("VERSCHIEBEN / TAUSCHEN",12,151,456,45);touchButton("ENTFERNEN",12,208,456,45);
  } else if(touchMode==17) {
    menuTop("ZIELPLATZ WAEHLEN");
    for(int page=0;page<PAGE_COUNT;++page)touchButton("SEITE "+String(page+1),12+page*156,46,144,38,page==moveTargetPage?BUTTON_BLUE:AMBER);
    for(int pos=0;pos<pageSlots[moveTargetPage];++pos){int i=moveTargetPage*STOPS_PER_PAGE+pos;touchButton(String(moveTargetPage+1)+"."+String(pos+1)+" "+clip(stops[i].length()?stops[i]:"Frei",28),12,94+pos*40,456,36,i==moveSource?BUTTON_BLUE:AMBER);}
  } else if(touchMode==18) {
    menuTop("WI-FI AUSWAHL");
    drawLine(clip(networks[wifiEdit].ssid,36),12,51,WHITE,2);
    touchButton("VERBINDEN",12,100,456,48);touchButton("ENTFERNEN",12,166,456,48);
    drawLine(clip(menuNotice,70),12,250,WHITE,1);
  } else if(touchMode==19) {
    menuTop("NETZ ENTFERNEN?");
    drawLine(clip(networks[wifiEdit].ssid,36),12,60,WHITE,2);
    drawLine("Letzte bewaehrte Verbindung ist geschuetzt.",12,110,WHITE,1);
    touchButton("JA, ENTFERNEN",12,169,222,47);touchButton("ABBRECHEN",246,169,222,47);
  } else if(touchMode==20||touchMode==21) {
    menuTop(touchMode==20?"WLAN NAME":"WLAN PASSWORT");
    String shown=inputText;
    if(touchMode==21){shown="";for(size_t i=0;i<inputText.length();++i)shown+='*';}
    drawLine(clip(shown,36),12,45,WHITE,2);
    for(int r=0;r<4;++r){String row=keyboardRow(r);for(int c=0;c<(int)row.length();++c)touchButton(row.substring(c,c+1),12+c*46,73+r*39,42,35);}
    touchButton("abc/#+",12,232,105,34);touchButton("LEER",125,232,100,34);touchButton("DEL",233,232,100,34);touchButton(touchMode==20?"WEITER":"SICHERN",341,232,127,34);
    const char *um[]={"AE","OE","UE","SS"};for(int i=0;i<4;++i)touchButton(um[i],12+i*114,274,106,32);
  } else if(touchMode==23) {
    menuTop("BATERIE");
    drawLine(batteryEnabled?"ANZEIGE EIN":"ANZEIGE AUS",12,60,WHITE,2);
    touchButton(batteryEnabled?"AUSSCHALTEN":"EINSCHALTEN",12,112,456,50);
    touchButton(batteryWarningEnabled?"WARNUNG EIN":"WARNUNG AUS",12,172,456,40);
    touchButton("-",12,224,70,44);
    touchButton("+",398,224,70,44);
    drawLine(batteryLowMv?"SCHWELLE "+String(batteryLowMv/1000.0f,2)+" V":"SCHWELLE --",98,238,AMBER,2);
    drawLine("BAT+: "+(batteryVoltage()>0?String(batteryVoltage(),2)+" V":"--")+"  | Schritt 0.05 V",12,280,WHITE,1);
    drawLine("Automatisch gespeichert. Passend zur Batterie waehlen.",12,300,WHITE,1);
  } else if(touchMode==22) {
    menuTop("NACHTZEITEN");
    drawLine("BEGINN "+clockMinutes(nightStart),12,54,WHITE,2);
    drawLine("ENDE   "+clockMinutes(nightEnd),12,163,WHITE,2);
    const char *lab[]={"-1H","+1H","-1M","+1M"};
    for(int r=0;r<2;++r)for(int c=0;c<4;++c)touchButton(lab[c],12+c*117,85+r*109,105,45);
  }
}
void eraseUtf8(String &s) { if(!s.length())return;int p=s.length()-1;while(p>0&&((uint8_t)s[p]&0xc0)==0x80)--p;s.remove(p); }
bool extendedTap(int x,int y) {
  if(touchMode<10) return false;
  if(y>=4&&y<31&&x>=352&&x<468) {
    if(touchMode==10){closeTouchMenu();return true;}
    if(touchMode==21){touchMode=20;inputText=pendingSsid;}
    else if(touchMode==24||touchMode==20||touchMode==18||touchMode==19){touchMode=12;inputText="";pendingSsid="";}
    else if(touchMode==16||touchMode==17){touchMode=11;}
    else if(touchMode==22||touchMode==25){touchMode=13;}
    else if(touchMode==26){touchMode=25;}
    else touchMode=10;
    drawExtendedMenu();return true;
  }
  if(touchMode==25) {
    if(x>=12&&x<468&&y>=46&&y<278&&(y-46)%48<40) {
      int row=(y-46)/48;
      if(row==0){touchMode=26;drawExtendedMenu();return true;}
      if(row==1)introMode=(introMode+1)%3;
      if(row==2){
        if(x<78){if(autoPageSeconds>0)--autoPageSeconds;}
        else if(x>=402){if(autoPageSeconds<60)++autoPageSeconds;}
        else if(x>=86&&x<394)autoPageSeconds=autoPageSeconds?0:15;
        else return true;
      }
      if(row==3){homeCity=activeCity;homePage=activePage;}
      if(row==4){goHome();return true;}
      saveComfortSettings();drawExtendedMenu();
    }return true;
  }
  if(touchMode==26) {
    if(x>=12&&x<468&&y>=43&&y<302&&(x-12)%234<222&&(y-43)%37<33){
      int city=((y-43)/37)*2+(x-12)/234;
      if(city<CITY_COUNT){favoriteCities^=uint16_t(1)<<city;saveComfortSettings();drawExtendedMenu();}
    }return true;
  }
  if(touchMode==23){if(x>=12&&x<468&&y>=112&&y<162){setBatteryEnabled(!batteryEnabled);drawExtendedMenu();}else if(x>=12&&x<468&&y>=172&&y<212){setBatteryWarningEnabled(!batteryWarningEnabled);drawExtendedMenu();}else if(y>=224&&y<268&&((x>=12&&x<82)||(x>=398&&x<468))){int mv=batteryLowMv;if(x<82)mv=mv<=2500?0:mv-50;else mv=mv==0?2500:min(4300,mv+50);setBatteryThreshold(mv);drawExtendedMenu();}return true;}
  if(touchMode==10) {
    if(x>=12&&x<468&&y>=278&&y<312){touchMode=23;drawExtendedMenu();return true;}
    if(x<12||x>=468||y<46||y>=274)return true;
    int col=(x-12)/234,row=(y-46)/57;if((x-12)%234>=222||(y-46)%57>=49)return true;
    const int modes[]={12,13,11,4,14,0,15,11};int mode=modes[row*2+col];
    if(mode==0){lastRefresh=0;closeTouchMenu();return true;}
    touchMode=mode;if(mode==12)startWifiScan();if(mode==4)drawCityMenu();else drawExtendedMenu();return true;
  }
  if(touchMode==11){
    if(y>=46&&y<84&&x>=12&&x<468){selectPage((x-12)/156);lastRefresh=0;closeTouchMenu();return true;}
    if(y>=270&&y<308&&x>=12&&x<468){setPageSlots(activePage,pageSlots[activePage]%3+1);drawExtendedMenu();return true;}
    if(y>=94&&y<254&&x>=12&&x<468){int pos=(y-94)/40;if(pos<pageSlots[activePage]&&(y-94)%40<36){touchEditIndex=activePage*STOPS_PER_PAGE+pos;touchMode=16;drawExtendedMenu();}}return true;
  }
  if(touchMode==12){
    if(y>=46&&y<226&&x>=12&&x<468){int i=(y-46)/45;if((y-46)%45<40&&networks[i].ssid[0]){wifiEdit=i;touchMode=18;menuNotice="";drawExtendedMenu();}}
    else if(y>=232&&y<272&&x>=12&&x<468){if(networkCount()>=4 && x>=234){menuNotice="Speicher voll. Erst ein unbenoetigtes Netz entfernen.";}else{touchMode=x<234?24:20;inputText="";pendingSsid="";keyboardPage=0;if(touchMode==24)startWifiScan();}drawExtendedMenu();}return true;
  }
  if(touchMode==24){
    if(y>=243&&y<287) {
      if(x>=12&&x<78)nearbyPage=max(0,nearbyPage-1);
      else if(x>=86&&x<306)startWifiScan(true);
      else if(x>=314&&x<380)nearbyPage=min(max(0,(nearbyCount-1)/4),nearbyPage+1);
      else if(x>=388&&x<468){touchMode=20;inputText="";pendingSsid="";keyboardPage=0;}
      drawExtendedMenu();
    } else if(!wifiScanning && x>=12&&x<468&&y>=46&&y<238&&(y-46)%48<44) {
      int i=nearbyPage*4+(y-46)/48;if(i<nearbyCount){
        for(int j=0;j<4;++j) if(nearbyWifi[i].ssid==networks[j].ssid){wifiEdit=j;touchMode=18;drawExtendedMenu();return true;}
        if(networkCount()>=4){touchMode=12;menuNotice="Speicher voll. Erst ein Netz entfernen.";drawExtendedMenu();return true;}
        pendingSsid=nearbyWifi[i].ssid;inputText="";keyboardPage=0;touchMode=21;drawExtendedMenu();}
    }return true;
  }
  if(touchMode==13){
    if(x>=12&&x<468&&y>=276&&y<312){touchMode=25;drawExtendedMenu();return true;}
    if(!pwmReady)return true;
    if(x>=20&&x<=460&&((y>=67&&y<87)||(y>=126&&y<146))){int v=constrain((x-20)*100/440,5,100);if(y<100)brightness=v;else nightBrightness=v;saveDisplaySettings();drawExtendedMenu();}
    else if(y>=163&&y<199){if(x>=12&&x<234){nightEnabled=!nightEnabled;saveDisplaySettings();drawExtendedMenu();}else if(x>=246&&x<468){touchMode=22;drawExtendedMenu();}}return true;
  }
  if(touchMode==14){if(y>=287&&x<234)drawExtendedMenu();return true;}
  if(touchMode==15){if(y>=246&&y<291){lastRefresh=0;closeTouchMenu();}return true;}
  if(touchMode==16){
    if(y>=94&&y<139){touchMode=1;touchQuery="";drawTouchMenu();}
    else if(y>=151&&y<196){moveSource=touchEditIndex;moveTargetPage=activePage;touchMode=17;drawExtendedMenu();}
    else if(y>=208&&y<253&&stops[touchEditIndex].length()){touchMode=3;drawTouchMenu();}return true;
  }
  if(touchMode==17){
    if(y>=46&&y<84&&x>=12&&x<468&&(x-12)%156<144){moveTargetPage=(x-12)/156;drawExtendedMenu();return true;}
    if(y>=94&&y<254&&x>=12&&x<468&&(y-94)%40<36){int target=moveTargetPage*STOPS_PER_PAGE+(y-94)/40;if(slotEnabled(target)){if(target!=moveSource){String old=stops[target];stops[target]=stops[moveSource];stops[moveSource]=old;saveStops();selectPage(target/STOPS_PER_PAGE);lastRefresh=0;}closeTouchMenu();}}return true;
  }
  if(touchMode==18){if(y>=100&&y<148){chooseNetwork(wifiEdit);closeTouchMenu();}else if(y>=166&&y<214){touchMode=19;drawExtendedMenu();}return true;}
  if(touchMode==19){if(y>=169&&y<216){if(x>=12&&x<234){menuNotice=deleteNetwork(wifiEdit)?"Netz entfernt":"Geschuetzt: letzte gespeicherte/bewaehrte Verbindung";touchMode=12;}else if(x>=246&&x<468)touchMode=18;drawExtendedMenu();}return true;}
  if(touchMode==20||touchMode==21){
    int limit=touchMode==20?32:63;
    if(y>=73&&y<229&&x>=12&&x<472&&(y-73)%39<35&&(x-12)%46<42){int row=(y-73)/39,col=(x-12)/46;String keys=keyboardRow(row);if(col<(int)keys.length()&&(int)inputText.length()<limit)inputText+=keys[col];}
    else if(y>=232&&y<266){
      if(x>=12&&x<117)keyboardPage=(keyboardPage+1)%3;
      else if(x>=125&&x<225&&(int)inputText.length()<limit)inputText+=' ';
      else if(x>=233&&x<333)eraseUtf8(inputText);
      else if(x>=341&&x<468){
        if(touchMode==20){if(inputText.length()){pendingSsid=inputText;inputText="";touchMode=21;}}
        else{int i=saveNetwork(pendingSsid,inputText);inputText="";pendingSsid="";if(i>=0){chooseNetwork(i);closeTouchMenu();return true;}touchMode=12;menuNotice="Nicht gespeichert: Passwort/SSID oder geschuetztes Netz.";}
      }
    }else if(y>=274&&y<306&&x>=12&&x<468){int i=(x-12)/114;const char *lower[]={"ä","ö","ü","ß"};const char *upper[]={"Ä","Ö","Ü","ß"};String c=keyboardPage==1?upper[i]:lower[i];if(inputText.length()+c.length()<=(size_t)limit)inputText+=c;}
    drawExtendedMenu();return true;
  }
  if(touchMode==22){int r=y>=85&&y<130?0:y>=194&&y<239?1:-1;if(r>=0&&x>=12&&x<468&&(x-12)%117<105){const int delta[]={-60,60,-1,1};int &v=r==0?nightStart:nightEnd;v=(v+delta[(x-12)/117]+1440)%1440;saveDisplaySettings();drawExtendedMenu();}return true;}
  return true;
}


void setupComfortRoutes() {
  server.on("/home",HTTP_POST,[](){goHome();redirectHome();});
  server.on("/comfort",HTTP_GET,[](){
    String h=pageStart("Komfort")+"<section class='card'><form method='post' action='/comfort'><h2>Favoriten</h2>";
    for(int city=0;city<CITY_COUNT;++city)h+="<label><input type='checkbox' name='fav"+String(city)+"' value='yes'"+String((favoriteCities>>city)&1?" checked":"")+"> "+escapeHtml(CITY_QUERIES[city])+"</label>";
    h+="<label>Intro</label><select name='intro'>";
    const char* modes[]={"Aus","Nur beim Start","Beim Start und Stadtwechsel"};
    for(int i=0;i<3;++i)h+="<option value='"+String(i)+"'"+String(introMode==i?" selected":"")+">"+modes[i]+"</option>";
    h+="</select><label>Automatischer Seitenwechsel: Sekunden (0 = Aus)</label><input name='auto' type='number' inputmode='numeric' min='0' max='60' step='1' required value='"+String(autoPageSeconds)+"'>";
    h+="<p>1–60 Sekunden, in Einzelschritten. 0 schaltet aus. Leere Seiten werden übersprungen. Pause im Menü und Intro. Manuelle Auswahl startet die Wartezeit neu.</p><label>Home-Stadt</label><select name='home'>";
    h+="<option value='255'"+String(homeCity==255?" selected":"")+">Nicht gesetzt</option>";
    for(int city=0;city<CITY_COUNT;++city)h+="<option value='"+String(city)+"'"+String(homeCity==city?" selected":"")+">"+escapeHtml(CITY_QUERIES[city])+"</option>";
    h+="</select><label>Home-Seite</label><select name='page'>";
    for(int i=0;i<PAGE_COUNT;++i)h+="<option value='"+String(i)+"'"+String(homePage==i?" selected":"")+">"+String(i+1)+"</option>";
    sendPage(h+"</select><button>Speichern</button></form></section>");
  });
  server.on("/comfort",HTTP_POST,[](){
    int intro,seconds,city,page;
    if(!numericArg("intro",0,2,intro)||!numericArg("auto",0,60,seconds)||!validAutoSeconds(seconds)||!numericArg("home",0,255,city)||(city>=CITY_COUNT&&city!=255)||!numericArg("page",0,PAGE_COUNT-1,page)){showMessage("Ungueltige Komforteinstellungen",400);return;}
    uint16_t favorites=0;
    for(int i=0;i<CITY_COUNT;++i)if(server.arg("fav"+String(i))=="yes")favorites|=uint16_t(1)<<i;
    favoriteCities=favorites;introMode=intro;autoPageSeconds=seconds;homeCity=city;homePage=page;saveComfortSettings();
    if(touchMenuOpen){if(touchMode==4)drawCityMenu();else drawExtendedMenu();}else{++touchExitRevision;drawBoard();}
    server.sendHeader("Location","/comfort");server.send(303);
  });
}
