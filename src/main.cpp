#include <Arduino.h>
#include <ArduinoJson.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <BleCombo.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include "index.h"
#include "default_script.h"


// PINS / DISPLAY
#define I2C_SDA 21
#define I2C_SCL 22
#define BTN_PIN 16
#define BTN_LEVEL LOW

// WIFI (STA first, AP fallback)
#define WIFI_SSID "WifiSSID"
#define WIFI_PASS "password123"

U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);
WebServer server(80);

static inline void ui(const char* l1, const char* l2 = nullptr) {
  oled.clearBuffer();
  const int W = oled.getDisplayWidth();
  const int H = oled.getDisplayHeight();

  auto drawCentered = [&](const char* text, const uint8_t* font, int baselineY) {
    if (!text || !*text) return;
    oled.setFont(font);
    int x = (W - oled.getStrWidth(text)) / 2;
    if (x < 0) x = 0;
    oled.drawStr(x, baselineY, text);
  };

  const bool hasL2 = (l2 && *l2);

  if (l1 && *l1 && !hasL2) {
    oled.setFont(u8g2_font_10x20_tf);
    const int a = oled.getAscent();
    const int d = oled.getDescent();
    const int textH = a - d;

    const int SINGLELINE_Y_OFFSET = 5;
    int baselineY = ((H - textH) / 2) + a + SINGLELINE_Y_OFFSET;

    int top = baselineY - a;
    int bottom = baselineY - d;
    if (top < 0) baselineY += -top;
    if (bottom > (H - 1)) baselineY -= (bottom - (H - 1));

    int x = (W - oled.getStrWidth(l1)) / 2;
    if (x < 0) x = 0;
    oled.drawStr(x, baselineY, l1);
  } else {
    drawCentered(l1, u8g2_font_10x20_tf, 14);
    drawCentered(l2, u8g2_font_7x13_tf, 31);
  }

  oled.sendBuffer();
}

static String ipToStr(IPAddress ip) {
  char buf[32];
  snprintf(buf, sizeof(buf), "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
  return String(buf);
}

// ======== SCRIPT STORAGE ========
const char* SCRIPT_PATH = "/script.json";
String currentScript;

bool loadScriptFromFS() {
  if (!LittleFS.exists(SCRIPT_PATH)) return false;
  File f = LittleFS.open(SCRIPT_PATH, "r");
  if (!f) return false;
  currentScript = f.readString();
  f.close();
  return currentScript.length() > 0;
}

bool saveScriptToFS(const String& s) {
  File f = LittleFS.open(SCRIPT_PATH, "w");
  if (!f) return false;
  f.print(s);
  f.close();
  currentScript = s;
  return true;
}

// ======== SCRIPT ENGINE ========
JsonDocument gDoc;
JsonArray gSteps;
String gMode = "relative";
int gRepeat = 1;

static inline void typeText(const char* s) {
  while (*s) {
    Keyboard.print(*s++);
    delay(3);
  }
}

static bool mapKeyToken(const String& in, uint8_t& code, bool& printable, char& ch) {
  String n = in;
  n.trim();
  n.toUpperCase();
  printable = false;
  ch = 0;
  code = 0;

  if (n.length() == 1) {
    ch = (char)n[0];
    printable = true;
    return true;
  }

  if (n == "ENTER" || n == "RETURN") { code = KEY_RETURN; return true; }
  if (n == "TAB") { code = KEY_TAB; return true; }
  if (n == "ESC" || n == "ESCAPE") { code = KEY_ESC; return true; }
  if (n == "BACKSPACE") { code = KEY_BACKSPACE; return true; }
  if (n == "SPACE") { printable = true; ch = ' '; return true; }
  if (n == "UP") { code = KEY_UP_ARROW; return true; }
  if (n == "DOWN") { code = KEY_DOWN_ARROW; return true; }
  if (n == "LEFT") { code = KEY_LEFT_ARROW; return true; }
  if (n == "RIGHT") { code = KEY_RIGHT_ARROW; return true; }
  if (n == "HOME") { code = KEY_HOME; return true; }
  if (n == "END") { code = KEY_END; return true; }
  if (n == "PGUP" || n == "PAGEUP") { code = KEY_PAGE_UP; return true; }
  if (n == "PGDN" || n == "PAGEDOWN") { code = KEY_PAGE_DOWN; return true; }
  if (n == "DEL" || n == "DELETE") { code = KEY_DELETE; return true; }
  if (n == "INS" || n == "INSERT") { code = KEY_INSERT; return true; }

  return false;
}

static bool isModifier(const String& in, uint8_t& modCode) {
  String n = in;
  n.trim();
  n.toUpperCase();
  if (n == "CTRL" || n == "CONTROL") { modCode = KEY_LEFT_CTRL; return true; }
  if (n == "SHIFT") { modCode = KEY_LEFT_SHIFT; return true; }
  if (n == "ALT" || n == "OPTION") { modCode = KEY_LEFT_ALT; return true; }
  if (n == "WIN" || n == "WINDOWS" || n == "GUI" || n == "CMD" || n == "META") {
    modCode = KEY_LEFT_GUI;
    return true;
  }
  return false;
}

static int splitPlus(const String& s, String out[], int maxOut) {
  int count = 0;
  String cur;
  for (size_t i = 0; i < s.length(); ++i) {
    if (s[i] == '+') {
      if (count < maxOut) out[count++] = cur;
      cur = "";
    } else {
      cur += s[i];
    }
  }
  if (cur.length() && count < maxOut) out[count++] = cur;
  return count;
}

static bool sendComboString(const String& combo) {
  if (!combo.length()) return false;

  String tokens[6];
  int tCount = splitPlus(combo, tokens, 6);
  if (tCount <= 0) return false;

  uint8_t mods[4];
  int mCount = 0;
  uint8_t keyCode = 0;
  bool printable = false;
  char ch = 0;
  bool haveKey = false;

  for (int i = 0; i < tCount; ++i) {
    uint8_t modCode = 0;
    if (isModifier(tokens[i], modCode)) {
      bool exists = false;
      for (int j = 0; j < mCount; ++j)
        if (mods[j] == modCode) { exists = true; break; }
      if (!exists && mCount < 4) mods[mCount++] = modCode;
      continue;
    }
    uint8_t kc = 0; bool pr = false; char c = 0;
    if (!mapKeyToken(tokens[i], kc, pr, c)) return false;
    keyCode = kc; printable = pr; ch = c; haveKey = true;
  }

  if (!haveKey && mCount > 0) {
    for (int i = 0; i < mCount; ++i) Keyboard.press(mods[i]);
    delay(8);
    for (int i = mCount - 1; i >= 0; --i) Keyboard.release(mods[i]);
    return true;
  }

  if (!haveKey) return false;

  for (int i = 0; i < mCount; ++i) Keyboard.press(mods[i]);
  delay(4);

  if (printable) {
    Keyboard.press((uint8_t)ch);
    delay(6);
    Keyboard.release((uint8_t)ch);
  } else {
    Keyboard.press(keyCode);
    delay(6);
    Keyboard.release(keyCode);
  }

  delay(4);
  for (int i = mCount - 1; i >= 0; --i) Keyboard.release(mods[i]);
  delay(2);
  return true;
}

static inline void mouseMoveSmooth(int dx, int dy, int stepDelay = 0) {
  int sx = (dx > 0) - (dx < 0);
  int sy = (dy > 0) - (dy < 0);
  int ax = abs(dx), ay = abs(dy), n = max(ax, ay);
  for (int i = 0; i < n; ++i) {
    int mx = (i < ax) ? sx : 0;
    int my = (i < ay) ? sy : 0;
    if (mx || my) Mouse.move(mx, my, 0);
    delay(stepDelay);
  }
}

static inline bool compileScript(const String& json) {
  gDoc.clear();
  auto err = deserializeJson(gDoc, json);
  if (err) { Serial.printf("[JSON] %s\n", err.c_str()); return false; }
  gMode = (const char*)(gDoc["mode"] | "relative");
  gRepeat = gDoc["repeat"] | 1;
  gSteps = gDoc["steps"].as<JsonArray>();
  return !gSteps.isNull();
}

static inline void runSteps(JsonArray steps, const String& mode) {
  for (JsonObject step : steps) {
    String action = step.begin()->key().c_str();
    JsonVariant params = step.begin()->value();

    if (action == "wait") {
      delay((int)params["ms"] | 0);
    } else if (action == "type") {
      typeText((const char*)(params["text"] | ""));
    } else if (action == "key") {
      if (params["code"].is<const char*>()) {
        String codeStr = (const char*)params["code"];
        if (!sendComboString(codeStr)) {
          uint8_t dummy = 0; bool pr = false; char ch = 0;
          if (mapKeyToken(codeStr, dummy, pr, ch)) {
            if (pr) Keyboard.write((uint8_t)ch);
            else { Keyboard.press(dummy); delay(6); Keyboard.release(dummy); }
          }
        }
      } else if (params["code"].is<int>()) {
        Keyboard.write((uint8_t)params["code"].as<int>());
      }
    } else if (action == "move") {
      int dx = params["dx"] | 0, dy = params["dy"] | 0;
      mouseMoveSmooth(dx, dy);
    } else if (action == "click") {
      Mouse.click(MOUSE_LEFT);
    } else if (action == "scroll") {
      int dy = params["dy"] | 0, ticks = dy / 40, st = (ticks >= 0) ? 1 : -1;
      for (int i = 0; i != ticks; i += st) { Mouse.move(0, 0, st); delay(6); }
      int dur = params["duration"] | 0;
      if (dur) delay(dur);
    }
  }
}

void handleRoot() { server.send_P(200, "text/html; charset=utf-8", PAGE_INDEX); }
void handleGet() { server.send(200, "application/json; charset=utf-8", currentScript); }

void handleSave() {
  String body = server.arg("plain");
  if (body.isEmpty()) { server.send(400, "text/plain", "Empty body"); return; }
  if (!compileScript(body)) { server.send(400, "text/plain", "JSON parse error"); return; }
  if (!saveScriptToFS(body)) { server.send(500, "text/plain", "FS write failed"); return; }

  ui("SAVED", "Script changed");
  server.send(200, "text/plain", "SAVED");
  Serial.println("[WEB] Script saved");
}

void handleRun() {
  if (gSteps.isNull() && !compileScript(currentScript)) {
    server.send(400, "text/plain", "No valid script");
    return;
  }
  server.send(200, "text/plain", "RUN");
  Serial.println("[WEB] Run requested");
  for (int r = 0; r < gRepeat; ++r) runSteps(gSteps, gMode);
}

void handleNotFound() { server.send(404, "text/plain", "Not found"); }

// ======== SETUP / LOOP ========
void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);
  oled.begin();
  ui("BOOT", "");

  if (!LittleFS.begin(true)) {
    Serial.println("[FS] Mount failed");
  } else {
    if (!loadScriptFromFS()) {
      currentScript = DEFAULT_SCRIPT;
      if (saveScriptToFS(currentScript)) {
        Serial.println("[FS] Created /script.json with default content");
      } else {
        Serial.println("[FS] Failed to create /script.json");
      }
    }
    if (!compileScript(currentScript)) {
      Serial.println("[FS] Script compile failed, using DEFAULT");
      compileScript(DEFAULT_SCRIPT);
    }
  }

  pinMode(BTN_PIN, INPUT_PULLUP);

  Keyboard.begin();
  Mouse.begin();
  ui("BT READY", "Press button");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("[WIFI] Connecting to %s ...\n", WIFI_SSID);
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 7000) { delay(100); }

  if (WiFi.status() == WL_CONNECTED) {
    IPAddress ip = WiFi.localIP();
    Serial.printf("[WIFI] Connected: %s\n", ipToStr(ip).c_str());
    ui("WEB", ipToStr(ip).c_str());
  } else {
    Serial.println("[WIFI] STA failed; starting AP");
    WiFi.mode(WIFI_AP);
    WiFi.softAP("TestBot-AP", "esp32test");
    IPAddress ip = WiFi.softAPIP();
    Serial.printf("[AP] SSID: TestBot-AP  PASS: esp32test  IP: %s\n", ipToStr(ip).c_str());
    ui("AP MODE", ipToStr(ip).c_str());
  }

  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/get", HTTP_GET, handleGet);
  server.on("/api/save", HTTP_POST, handleSave);
  server.on("/api/run", HTTP_POST, handleRun);
  server.onNotFound(handleNotFound);
  server.begin();
}

void loop() {
  server.handleClient();

  static bool prevConn = false;
  bool nowConn = Keyboard.isConnected();
  if (nowConn != prevConn) {
    prevConn = nowConn;
    if (nowConn) { ui("READY", "Press button"); Serial.println("[BLE] Connected"); }
    else { Serial.println("[BLE] Disconnected"); }
  }

  static uint32_t lastKA = 0;
  if (nowConn && millis() - lastKA > 1500) { lastKA = millis(); Mouse.move(0, 0, 0); }

  static bool lastHigh = true;
  bool high = (digitalRead(BTN_PIN) != BTN_LEVEL);
  if (lastHigh && !high) {
    if (!nowConn) {
      ui("NO BT", "PAIR");
      Serial.println("[BTN] Press without BT");
    } else {
      ui("RUN", "Running...");
      Serial.println("[BTN] Running script");
      for (int r = 0; r < gRepeat; ++r) runSteps(gSteps, gMode);
      ui("DONE", "Script executed");
      Serial.println("[RUN] Done");
    }
  }
  lastHigh = high;

  delay(5);
}
