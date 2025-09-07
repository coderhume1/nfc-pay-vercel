// firmware/device-writer/src/main.cpp
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_PN532.h>
#include <Adafruit_NeoPixel.h>
#include "config.h"

// --- PN532 over I2C ---
Adafruit_PN532 nfc(PN532_I2C_ADDRESS);

// --- WS2812B ---
Adafruit_NeoPixel pixels(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// --- BUZZER (ESP32 LEDC PWM) ---
static const int BUZZ_CH = 2;     // LEDC channel
static const int BUZZ_RES = 10;   // bits
static const int BUZZ_BASE_FREQ = 2000; // Hz for passive buzzer default

// === Small utils ===
static void ledsFill(uint32_t color) {
  for (int i = 0; i < LED_COUNT; ++i) pixels.setPixelColor(i, color);
  pixels.show();
}

static uint32_t colorRGB(uint8_t r, uint8_t g, uint8_t b) {
  return pixels.Color(r, g, b);
}

static void ledsPulse(uint32_t color, int flashes=2, int onMs=100, int offMs=100) {
  for (int n=0; n<flashes; ++n) {
    ledsFill(color);
    delay(onMs);
    ledsFill(0);
    delay(offMs);
  }
}

// Buzzer control
static void buzzerInit() {
#if BUZZER_IS_PASSIVE
  ledcSetup(BUZZ_CH, BUZZ_BASE_FREQ, BUZZ_RES);
  ledcAttachPin(BUZZER_PIN, BUZZ_CH);
#else
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, BUZZER_ACTIVE_HIGH ? LOW : HIGH); // idle off
#endif
}

static void buzzerOn(int freq = BUZZ_BASE_FREQ) {
#if BUZZER_IS_PASSIVE
  // Generate tone
  ledcWriteTone(BUZZ_CH, freq);
#else
  digitalWrite(BUZZER_PIN, BUZZER_ACTIVE_HIGH ? HIGH : LOW);
#endif
}

static void buzzerOff() {
#if BUZZER_IS_PASSIVE
  ledcWrite(BUZZ_CH, 0); // stop PWM
#else
  digitalWrite(BUZZER_PIN, BUZZER_ACTIVE_HIGH ? LOW : HIGH);
#endif
}

static void beep(int ms=120, int freq=2000) {
  buzzerOn(freq);
  delay(ms);
  buzzerOff();
}

// Feedback patterns
static void fxBoot() {
  ledsPulse(colorRGB(0, 64, 64), 2, 120, 80);   // teal double blink
  beep(90, 1800);
}

static void fxWifiOK() {
  ledsPulse(colorRGB(0, 80, 0), 2, 80, 60);     // green blips
  beep(70, 2200);
}

static void fxNoPN532() {
  ledsPulse(colorRGB(80, 0, 80), 3, 150, 120);  // magenta triple
  for(int i=0;i<2;i++){ beep(120, 1200); delay(80); }
}

static void fxWaitingIdle() {
  // Dim blue idle
  ledsFill(colorRGB(0, 0, 12));
}

static void fxNewJob() {
  // Amber notify + double bip
  ledsPulse(colorRGB(80, 40, 0), 2, 120, 80);
  for(int i=0;i<2;i++){ beep(70, 2400); delay(60); }
}

static void fxSuccess() {
  // Green flashes + long beep
  ledsPulse(colorRGB(0, 80, 0), 3, 90, 60);
  beep(180, 2000);
}

static void fxError() {
  // Red flashes + 3 short beeps
  ledsPulse(colorRGB(90, 0, 0), 3, 140, 90);
  for(int i=0;i<3;i++){ beep(70, 1500); delay(70); }
}

// --- HTTP helpers ---
static String httpGet(const String& url, const String& headerKey = "", const String& headerVal = "", int* codeOut=nullptr) {
  HTTPClient http;
  http.begin(url);
  if (headerKey.length()) http.addHeader(headerKey, headerVal);
  int code = http.GET();
  String resp = http.getString();
  http.end();
  if (codeOut) *codeOut = code;
  return resp;
}

static String httpPostJson(const String& url, const String& json, int* codeOut=nullptr) {
  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  int code = http.POST(json);
  String resp = http.getString();
  http.end();
  if (codeOut) *codeOut = code;
  return resp;
}

// --- NFC write ---
static bool writeUriNdef(const String& uri) {
  Serial.print("[NFC] Writing URI: "); Serial.println(uri);
  uint8_t ndefprefix = 0x00; // no abbreviation
  if (!nfc.ntag2xx_WriteNDEFURI(ndefprefix, (char*)uri.c_str())) {
    Serial.println("[NFC] Write failed");
    return false;
  }
  Serial.println("[NFC] Write success");
  return true;
}

// --- Main ---
void setup() {
  Serial.begin(115200);
  delay(200);

  // LEDs & Buzzer
  pixels.begin();
  pixels.setBrightness(LED_BRIGHTNESS);
  ledsFill(0);
  buzzerInit();
  fxBoot();

  // WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("[WiFi] Connecting");
  while (WiFi.status() != WL_CONNECTED) { delay(250); Serial.print("."); }
  Serial.printf("\n[WiFi] IP: %s\n", WiFi.localIP().toString().c_str());
  fxWifiOK();

  // PN532
  Wire.begin();
  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.println("[PN532] Not found");
    fxNoPN532();
  } else {
    Serial.printf("[PN532] FW: 0x%08lx\n", versiondata);
  }

  // Bootstrap (register device)
  {
    String url = String(BASE_URL) + "/api/v1/bootstrap";
    int code = 0;
    String resp = httpGet(url, "x-device-key", DEVICE_KEY, &code);
    Serial.printf("[BOOTSTRAP] GET %s => %d\n", url.c_str(), code);
  }

  fxWaitingIdle();
}

unsigned long lastPoll = 0;

void loop() {
  // Idle status
  if (millis() - lastPoll < POLL_MS) {
    delay(30);
    return;
  }
  lastPoll = millis();

  // Poll for next job
  String url = String(BASE_URL) + "/api/v1/device/next";
  HTTPClient http;
  http.begin(url);
  http.addHeader("x-device-key", DEVICE_KEY);
  int code = http.GET();
  String body = http.getString();
  http.end();

  if (code == 204) {
    // No job—soft heartbeat
    pixels.setPixelColor(0, colorRGB(0,0,8));
    pixels.show();
    return;
  }
  Serial.printf("[NEXT] %d: %s\n", code, body.c_str());
  fxNewJob();

  // crude JSON parsing (no ArduinoJSON)
  int idStart = body.indexOf("\"id\":\"");
  int idEnd = body.indexOf("\"", idStart + 6);
  int uriStart = body.indexOf("\"uri\":\"");
  int uriEnd = body.indexOf("\"", uriStart + 7);
  if (idStart < 0 || uriStart < 0) {
    fxError();
    return;
  }
  String jobId = body.substring(idStart + 6, idEnd);
  String uri = body.substring(uriStart + 7, uriEnd);

  bool ok = writeUriNdef(uri);
  if (ok) fxSuccess(); else fxError();

  // Ack
  String ackUrl = String(BASE_URL) + "/api/v1/device/ack";
  String payload = String("{\"jobId\":\"") + jobId + "\",\"ok\":" + (ok ? "true" : "false") + "}";
  int ackCode = 0;
  String ackResp = httpPostJson(ackUrl, payload, &ackCode);
  Serial.printf("[ACK] %d: %s\n", ackCode, ackResp.c_str());

  fxWaitingIdle();
}
