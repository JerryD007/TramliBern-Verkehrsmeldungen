#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <Arduino_GFX_Library.h>
#include <time.h>
#include <SPI.h>
#include <XPT2046_Touchscreen.h>

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

constexpr uint16_t BLACK = 0x0000, AMBER = 0xFDE0, WHITE = 0xFFFF;
// Roter Seitenwechsel-Button, leicht rechts der Bildschirmmitte.
constexpr uint16_t BUTTON_RED = 0xF800;
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
int touchMode = 0; // 0 Tafel, 1 Tastatur, 2 Trefferliste
int touchEditIndex = -1;
String touchQuery;
String touchMatches[5];
int touchMatchCount = 0;
String rowCache[MAX_ROWS_PER_PAGE];
String timeCache[MAX_ROWS_PER_PAGE];
String stops[MAX_STOPS];
int stopCount = 0;
String wifiSsid, wifiPassword;
String statusText = "Startet";
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

// Nur der ABFAHRTEN-Button ist neu; Rest des Kopfs bleibt unveraendert.
void drawCityButton() {
  gfx->fillRoundRect(CITY_BUTTON_X, CITY_BUTTON_Y,
                     CITY_BUTTON_W, CITY_BUTTON_H, 4, BUTTON_BLUE);
  gfx->setTextSize(2);
  gfx->setTextColor(WHITE, BUTTON_BLUE);
  String name = CITY_LABELS[activeCity];
  int width = name.length() * 12;
  // Lange Stadtnamen passen im blauen Button mit Schriftgroesse 1.
  if (width > CITY_BUTTON_W - 10) {
    gfx->setTextSize(1);
    width = name.length() * 6;
  }
  gfx->setCursor(CITY_BUTTON_X + (CITY_BUTTON_W - width) / 2,
                 CITY_BUTTON_Y + (CITY_BUTTON_H - (width == (int)name.length()*6 ? 8 : 16)) / 2);
  gfx->print(name);
}

void drawPageButton() {
  gfx->fillRoundRect(PAGE_BUTTON_X, PAGE_BUTTON_Y,
                     PAGE_BUTTON_W, PAGE_BUTTON_H, 4, BUTTON_RED);
  gfx->setTextSize(2);
  gfx->setTextColor(WHITE, BUTTON_RED);
  // Standardschrift 6 Pixel/Zeichen bei Textgroesse 1; 9 Zeichen = 108 Pixel.
  const int textWidth = 9 * 12;
  gfx->setCursor(PAGE_BUTTON_X + (PAGE_BUTTON_W - textWidth) / 2,
                 PAGE_BUTTON_Y + (PAGE_BUTTON_H - 16) / 2);
  gfx->print("ABFAHRTEN");
}

void drawHeader() {
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
  if (force) { drawCityButton(); drawPageButton(); }
  if (force || clock != previousClock) {
    drawOpaque(clock, 385, 8, 5, WHITE);
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
    if (force || fill != previousFill) {
      gfx->fillRect(448, 12, 25, 13, BLACK);
      if (fill >= 0) {
        gfx->drawRect(448, 12, 20, 12, WHITE);
        gfx->fillRect(469, 15, 3, 6, WHITE);
        if (fill > 0) gfx->fillRect(450, 14, fill, 8, AMBER);
      }
      previousFill = fill;
    }
  }
}

// Auf Seite 1 und 2 wie bisher: je zwei Haltestellen, drei Fahrten.
// Auf Seite 3: genau eine Haltestelle mit sieben Fahrten.
void drawDepartureRows() {
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
  if (touchMenuOpen) return;
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
  gfx->fillScreen(BLACK);
  gfx->drawFastHLine(10, 34, 460, AMBER);
  drawHeader();
  if (WiFi.status() != WL_CONNECTED) {
    drawLine("WLAN nicht verbunden", 12, 60, WHITE);
    drawLine("Setup: TRAMLI-BERN-2", 12, 93, AMBER);
    drawLine("Browser: 192.168.4.1", 12, 123, WHITE);
  } else {
    for (int position = 0; position < (activePage == 2 ? 1 : STOPS_PER_PAGE); ++position) {
      int i = activePage * STOPS_PER_PAGE + position;
      if (!stops[i].length()) continue;
      String name = stops[i];
      name.replace(String(CITY_QUERIES[activeCity]) + ", ", "");
      drawLine(clip(name, 38), 12, 43 + position * 110, WHITE, 2);
    }
    drawDepartureRows();
  }
  gfx->drawFastHLine(10, 267, 460, AMBER);
  String address = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
  drawLine("Konfiguration: " + address, 12, 276, WHITE, 1);
  for (int page = 0; page < PAGE_COUNT; ++page) {
    int x = 320 + page * 50;
    gfx->fillRect(x, 270, 46, 24, page == activePage ? AMBER : BLACK);
    gfx->drawRect(x, 270, 46, 24, AMBER);
    drawLine(String(page + 1), x + 17, 274, page == activePage ? BLACK : WHITE, 2);
  }
  gfx->drawFastHLine(10, 295, 460, AMBER);
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
  String html = R"TRAMLI(<!doctype html><html lang="de-CH"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><meta name="theme-color" content="#aa3528"><title>Traemli Schweiz</title><style>
*{box-sizing:border-box}body{margin:0;background:#f4f3ed;color:#23332f;font:17px system-ui,-apple-system,sans-serif}header{background:white;padding:22px;border-bottom:1px solid #dce0d5}header div,main{max-width:800px;margin:auto}header b{font-size:25px}header span{display:block;font-size:12px;letter-spacing:2px;color:#70796c;margin-top:5px}main{padding:25px 16px 40px}h1{font-size:32px;margin:10px 0 6px;overflow-wrap:anywhere}h2{font-size:22px;margin:0 0 16px}p{line-height:1.55}small,.muted{color:#667366}.card{background:white;border:1px solid #dce1d4;border-radius:18px;padding:22px;margin:18px 0}button,.button{font:inherit;font-weight:650;background:#aa3528;color:white;border:0;border-radius:10px;padding:13px 17px;min-height:48px;cursor:pointer;text-decoration:none;display:inline-block}a{color:#983023}button:focus-visible,a:focus-visible,input:focus-visible{outline:3px solid #328676;outline-offset:3px}input{display:block;font:inherit;border:1px solid #c9d1c3;border-radius:10px;padding:13px;width:100%;margin:7px 0 16px;min-height:48px}label{display:block;font-size:15px;font-weight:600;margin-top:12px}ol,ul{list-style:none;padding:0;margin:0}li{display:flex;align-items:center;justify-content:space-between;gap:12px;padding:15px 0;border-bottom:1px solid #e5e8df;overflow-wrap:anywhere}li:last-child{border:0}.subtle{background:#edf0e7;color:#3e5143;font-size:14px;padding:10px 13px}.row{display:flex;gap:12px;flex-wrap:wrap;align-items:center}.row form{margin:0}nav{display:flex;gap:10px;margin-top:18px;flex-wrap:wrap}nav a{padding:10px 13px;background:white;border:1px solid #dae0d1;border-radius:9px;text-decoration:none;color:#354a3d}.status{padding:15px 18px;background:#e8eddf;border-radius:12px;font-size:15px}.hint{font-size:14px;color:#657262;line-height:1.6}.notice{background:#fff2cf;padding:15px;border-radius:12px}summary{cursor:pointer;font-weight:700;min-height:40px}footer{font-size:12px;color:#717b6c;margin-top:25px}.address{font-family:monospace;font-size:14px;display:block;color:#657262;margin-top:4px}.full{width:100%}form{margin:0} @media(max-width:420px){.card{padding:17px}h1{font-size:28px}li{align-items:flex-start}.row>*{flex-grow:1}}
</style></head><body><header><div><b>▤ Trämli Bern</b><span>DEINE ABFAHRTSTAFELN</span></div></header><main>)TRAMLI";
  html += "<h1>" + escapeHtml(title) + "</h1><p class='muted'>" + escapeHtml(boardLabel()) + " · " + escapeHtml(currentAddress()) + "</p>";
  html += "<nav aria-label='Navigation'><a href='/'>Haltestellen</a><a href='/boards'>Meine Boards</a><a href='/settings'>Einstellungen</a></nav>";
  return html;
}
String pageEnd() { return "<footer>Trämli Bern · Lokale Bedienung · Version SLIM TEST · Seite 1-2: je 2 Haltestellen / 3 Abfahrten · Seite 3: 1 Haltestelle / 7 Abfahrten</footer></main></body></html>"; }
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
  // TramliBern 3.0 TEST: Diagnose nur im lokalen Browser.
  // Meldungen werden nicht veraendert; keine unbestaetigten Linien-Zuordnungen.
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
    else html += "<form action='/search' method='get'><label for='q'>Name der Haltestelle</label><input id='q' name='q' type='text' minlength='2' maxlength='100' placeholder='z. B. Bahnhof' required><button class='full'>Haltestelle suchen</button></form>";
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
              html += "<option value='" + String(slot) + "'>Seite " + String(slot / STOPS_PER_PAGE + 1) + ", Platz " + String(slot % STOPS_PER_PAGE + 1) + "</option>";
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
    stops[index] = ""; saveStops(); lastRefresh = 0; redirectHome();
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
    html += "<section class='card'><details><summary>WLAN ändern</summary><p class='hint'>Nur ausfüllen, wenn du das WLAN wechseln möchtest. Nach dem Speichern startet das Board neu; seine IP-Adresse kann sich ändern.</p><form action='/wifi' method='post'><label for='ssid'>WLAN-Name</label><input id='ssid' name='ssid' maxlength='32' value='" + escapeHtml(wifiSsid) + "' required><label for='pass'>WLAN-Passwort neu eingeben</label><input id='pass' name='pass' type='password' maxlength='63' autocomplete='new-password'><p class='hint'>Leer lassen nur für ein WLAN ohne Passwort.</p><label><input type='checkbox' name='confirm' value='yes' required style='display:inline;width:auto;min-height:0'> WLAN-Wechsel und Neustart bestätigen</label><p><button class='full'>WLAN speichern und neu starten</button></p></form></details></section>";
    sendPage(html);
  });
  server.on("/device", HTTP_POST, []() {
    String name = server.arg("name"); name.trim();
    if (!name.length() || name.length() > 60) { showMessage("Bitte einen Namen mit höchstens 60 Zeichen eingeben."); return; }
    prefs.putString("boardname", name); redirectBoards();
  });
  server.on("/wifi", HTTP_POST, []() {
    String ssid = server.arg("ssid"), pass = server.arg("pass");
    if (server.arg("confirm") != "yes") { showMessage("Bitte WLAN-Wechsel bestätigen."); return; }
    if (ssid.length() && ssid.length() <= 32 && pass.length() <= 63) {
      prefs.putString("ssid", ssid); prefs.putString("pass", pass);
      sendPage(pageStart("WLAN gespeichert") + "<section class='card'><p>Das Board startet jetzt neu. Verbinde dein Handy mit dem gewählten WLAN und öffne die Adresse, die auf dem Display angezeigt wird.</p><p>Falls die Verbindung fehlschlägt: mit TRAMLI-BERN-2 verbinden und http://192.168.4.1 öffnen.</p></section>");
      delay(500); ESP.restart();
    } else showMessage("Ungültiger WLAN-Name oder Passwort.");
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
  gfx->drawRoundRect(x, y, w, h, 4, color);
  // Textgroesse 2 = 12 Pixel pro Zeichen.
  int textX = x + max(3, (w - (int)label.length() * 12) / 2);
  drawLine(label, textX, y + (h - 16) / 2, color, 2);
}

void drawTouchMenu() {
  gfx->fillScreen(BLACK);
  drawLine("HALTESTELLE AENDERN", 12, 6, AMBER, 2);
  gfx->drawFastHLine(10, 30, 460, AMBER);
  if (touchEditIndex < 0 || touchEditIndex >= MAX_STOPS) return;
  String oldName = stops[touchEditIndex].length() ? stops[touchEditIndex] : "Freier Platz";
  drawLine(clip(oldName, 36), 12, 38, WHITE, 1);
  touchButton("ABBRUCH", 352, 4, 116, 25, WHITE);
  if (touchMode == 1) {
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
    // Direct page selection also works for empty pages.
    if (y >= 270 && y < 295 && x >= 320 && x < 470) {
      selectPage((x - 320) / 50);
      lastRefresh = 0;
      drawBoard();
      return;
    }
    // Blauer Button: naechste Stadt. Die gespeicherten Haltestellen sind getrennt.
    if (x >= CITY_BUTTON_X && x < CITY_BUTTON_X + CITY_BUTTON_W &&
        y >= CITY_BUTTON_Y && y < CITY_BUTTON_Y + CITY_BUTTON_H) {
      changeCity((activeCity + 1) % CITY_COUNT);
      return;
    }
    // Tasten-Tippen schaltet Seite 1 -> 2 -> 3 -> 1; Swipe bleibt bestehen.
    if (x >= PAGE_BUTTON_X && x < PAGE_BUTTON_X + PAGE_BUTTON_W &&
        y >= PAGE_BUTTON_Y && y < PAGE_BUTTON_Y + PAGE_BUTTON_H) {
      selectPage((activePage + 1) % PAGE_COUNT);
      lastRefresh = 0;
      drawBoard();
      return;
    }
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
  pinMode(27, OUTPUT); digitalWrite(27, HIGH);
  pinMode(33, OUTPUT); digitalWrite(33, HIGH); // Touch-CS inaktiv
  pinMode(5, OUTPUT); digitalWrite(5, HIGH);   // SD-CS inaktiv
  gfx->begin(27000000);
  setupTouch();
  loadTouchCalibration();
  gfx->setTextWrap(false);
  gfx->fillScreen(BLACK);
  drawLine("Bern Abfahrtstafel", 12, 20, AMBER);
  loadSettings();
  WiFi.mode(WIFI_STA);
  if (wifiSsid.length()) {
    WiFi.begin(wifiSsid.c_str(), wifiPassword.c_str());
    uint32_t started = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - started < 12000) delay(200);
  }
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP("TRAMLI-BERN-2");
    gfx->fillScreen(BLACK);
    drawLine("WLAN: TRAMLI-BERN-2", 12, 32, AMBER);
    drawLine("Browser: 192.168.4.1", 12, 70, WHITE);
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
  checkSwipe();
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

