#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <SPI.h>
#include <Adafruit_PN532.h>

#define LED_PIN 2
#define FACTORY_PIN 34  // hold LOW at boot for reset message (no stored creds in minimal sketch)
#define BUZZER_PIN 15

// PN532 SPI
#define PN532_SCK   18
#define PN532_MOSI  23
#define PN532_MISO  19
#define PN532_SS     5
#define PN532_RST   27

Adafruit_PN532 nfc(PN532_SS);

const char* WIFI_SSID = "YOUR_WIFI";
const char* WIFI_PASS = "YOUR_PASS";
const char* API_KEY   = "esp32_test_api_key_123456";
const char* BASE_URL  = "https://YOUR-PROJECT.vercel.app";

void buzz(int ms){ tone(BUZZER_PIN, 2200, ms); }
void ledPending(){ digitalWrite(LED_PIN, millis()/500%2); }
void ledPaid(){ digitalWrite(LED_PIN, HIGH); }
void ledError(){ digitalWrite(LED_PIN, millis()/120%2); }

// --- NFC helpers (same as SoftAP version) ---
uint8_t uriPrefixCode(const String& url, String& rem) {
  struct { const char* pre; uint8_t code; } map[] = {
    {"http://www.", 0x01},{"https://www.",0x02},{"http://",0x03},{"https://",0x04}
  };
  for (auto &m: map) { size_t L=strlen(m.pre); if(url.startsWith(m.pre)){ rem=url.substring(L); return m.code; } }
  rem = url; return 0x00;
}
size_t buildURI_TLV(const String& url, uint8_t* out, size_t maxlen) {
  String rem; uint8_t id = uriPrefixCode(url, rem);
  size_t payloadLen = 1 + rem.length();
  size_t ndefLen = 3 + 1 + payloadLen;
  size_t tlvLen = 2 + ndefLen + 1;
  if (tlvLen > 255 || tlvLen > maxlen) return 0;
  size_t i=0;
  out[i++]=0x03; out[i++]=(uint8_t)ndefLen;
  out[i++]=0xD1; out[i++]=0x01; out[i++]=(uint8_t)payloadLen;
  out[i++]=0x55; out[i++]=id;
  for(size_t k=0;k<rem.length();k++) out[i++]=(uint8_t)rem[k];
  out[i++]=0xFE;
  return i;
}
bool writePagesPaced(const uint8_t* data, size_t len, uint8_t startPage=4) {
  uint8_t pageBuf[4]; size_t idx=0; uint8_t page=startPage;
  while(idx<len){
    memset(pageBuf,0x00,4);
    for(uint8_t k=0;k<4 && idx<len;k++) pageBuf[k]=data[idx++];
    if(!nfc.mifareultralight_WritePage(page,pageBuf)) return false;
    delay(12);
    page++;
  }
  memset(pageBuf,0x00,4);
  nfc.mifareultralight_WritePage(page,pageBuf);
  delay(12);
  return true;
}
bool writeNdefUrl(const String& url){
  uint8_t tlv[200];
  size_t L = buildURI_TLV(url, tlv, sizeof(tlv));
  if(!L) return false;
  return writePagesPaced(tlv, L, 4);
}

// --- HTTP helpers ---
bool httpsGet(const String& path, String& out){
  WiFiClientSecure client; client.setInsecure();
  HTTPClient http; String url = String(BASE_URL) + path;
  if(!http.begin(client, url)) return false;
  http.addHeader("X-API-Key", API_KEY);
  http.addHeader("X-Device-Id", WiFi.macAddress());
  int code = http.GET(); if(code>0) out = http.getString(); http.end();
  return code==200;
}
bool httpsPostJson(const String& path, const String& body, String& out){
  WiFiClientSecure client; client.setInsecure();
  HTTPClient http; String url = String(BASE_URL) + path;
  if(!http.begin(client, url)) return false;
  http.addHeader("X-API-Key", API_KEY);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-Device-Id", WiFi.macAddress());
  int code = http.POST(body); if(code>0) out = http.getString(); http.end();
  return code>=200 && code<300;
}

String terminalId, sessionId, checkoutUrl;

void setup(){
  pinMode(FACTORY_PIN, INPUT);
  if(digitalRead(FACTORY_PIN)==LOW){
    // In minimal sketch we don't store creds, so just notify
    Serial.begin(115200);
    Serial.println("[FACTORY] Detected (minimal). No stored settings to clear.");
    delay(1000);
  }
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  Serial.begin(115200);

  // NFC init
  pinMode(PN532_RST, OUTPUT);
  digitalWrite(PN532_RST, LOW); delay(10); digitalWrite(PN532_RST, HIGH); delay(10);
  nfc.begin();

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("[WiFi] Connecting to %s ...\n", WIFI_SSID);
  while(WiFi.status()!=WL_CONNECTED){ ledError(); delay(250); }
  Serial.println("[WiFi] Connected");
}

unsigned long lastPoll=0;
void loop(){
  if(terminalId.length()==0){
    String resp;
    if(httpsGet("/api/v1/bootstrap", resp)){
      terminalId = resp.substring(resp.indexOf("\"terminalId\":")+14);
      terminalId = terminalId.substring(terminalId.indexOf("\"")+1);
      terminalId = terminalId.substring(0, terminalId.indexOf("\""));
      int idx = resp.indexOf("\"checkoutUrl\":");
      if(idx>0){
        String t = resp.substring(idx+14);
        t = t.substring(t.indexOf("\"")+1);
        checkoutUrl = t.substring(0, t.indexOf("\""));
      }
    }
  }
  if(sessionId.length()==0 && terminalId.length()>0){
    String resp;
    if(httpsPostJson("/api/v1/sessions", "{}", resp)){
      int i = resp.indexOf("\"id\":");
      if(i>0){
        String t = resp.substring(i+5);
        t = t.substring(t.indexOf("\"")+1);
        sessionId = t.substring(0, t.indexOf("\""));
      }
      if(checkoutUrl.length()>0){
        if(writeNdefUrl(checkoutUrl)){ buzz(60); }
      }
    }
  }

  if(sessionId.length()>0 && millis()-lastPoll>1500){
    lastPoll = millis();
    String resp;
    if(httpsGet(String("/api/v1/sessions/")+sessionId, resp)){
      if(resp.indexOf("\"paid\"")>=0){ ledPaid(); buzz(120); delay(150); buzz(120); }
      else ledPending();
    } else ledError();
  }
}
