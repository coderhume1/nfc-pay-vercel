#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <SPI.h>
#include <Adafruit_PN532.h>

// -------- Pins --------
#define LED_PIN     2
#define BUZZER_PIN  15

// PN532 SPI (adjust if different)
#define PN532_SCK   18
#define PN532_MOSI  23
#define PN532_MISO  19
#define PN532_SS     5
#define PN532_RST   27

Adafruit_PN532 nfc(PN532_SS);

// -------- Config (edit) --------
const char* API_KEY   = "esp32_test_api_key_123456";
const char* BASE_URL  = "https://YOUR-PROJECT.vercel.app"; // <-- set your project
const char* AP_PASS   = "nfcsetup";

// -------- Preferences --------
Preferences prefs;
String wifiSsid, wifiPass;

// -------- SoftAP --------
WebServer server(80);
bool apMode = false;

String htmlIndex() {
  String s = "<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'/>"
             "<style>body{font-family:system-ui;padding:20px}input,button{padding:8px;margin:4px;width:100%;max-width:400px}"
             "label{display:block;margin-top:8px}</style></head><body>";
  s += "<h2>NFC-PAY Wi-Fi Setup</h2>";
  s += "<form method='POST' action='/save'>";
  s += "<label>SSID</label><input name='ssid'/>";
  s += "<label>Password</label><input name='pass' type='password'/>";
  s += "<button type='submit'>Save & Reboot</button>";
  s += "</form>";
  s += "<hr/><form method='POST' action='/factory-reset'><button>Factory Reset</button></form>";
  s += "</body></html>";
  return s;
}

void handleIndex(){ server.send(200, "text/html", htmlIndex()); }
void handleSave(){
  if(server.hasArg("ssid") && server.hasArg("pass")){
    prefs.putString("ssid", server.arg("ssid"));
    prefs.putString("pass", server.arg("pass"));
    server.send(200, "text/plain", "Saved. Rebooting...");
    delay(800);
    ESP.restart();
  } else {
    server.send(400, "text/plain", "Missing ssid/pass");
  }
}
void handleFactory(){
  prefs.clear();
  server.send(200, "text/plain", "Reset. Rebooting...");
  delay(500);
  ESP.restart();
}

void startAP(){
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  String ssid = "NFC-PAY-Setup-" + mac.substring(mac.length()-4);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid.c_str(), AP_PASS);
  IPAddress ip = WiFi.softAPIP();
  server.on("/", handleIndex);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/factory-reset", HTTP_POST, handleFactory);
  server.begin();
  apMode = true;
  Serial.printf("[AP] Started %s, http://%s\n", ssid.c_str(), ip.toString().c_str());
}

// -------- Status helpers --------
void ledPatternPending(){ digitalWrite(LED_PIN, millis()/500%2); }
void ledPatternPaid(){ digitalWrite(LED_PIN, HIGH); }
void ledPatternError(){ digitalWrite(LED_PIN, millis()/120%2); }

void buzz(int ms){ tone(BUZZER_PIN, 2200, ms); }

// -------- NFC helpers (URI NDEF TLV) --------
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

// -------- HTTP helpers --------
bool httpsGet(const String& path, String& out){
  WiFiClientSecure client; client.setInsecure();
  HTTPClient http;
  String url = String(BASE_URL) + path;
  if(!http.begin(client, url)) return false;
  http.addHeader("X-API-Key", API_KEY);
  http.addHeader("X-Device-Id", WiFi.macAddress());
  int code = http.GET();
  if(code>0){ out = http.getString(); }
  http.end();
  return code==200;
}
bool httpsPostJson(const String& path, const String& body, String& out, int* codeOut=nullptr){
  WiFiClientSecure client; client.setInsecure();
  HTTPClient http;
  String url = String(BASE_URL) + path;
  if(!http.begin(client, url)) return false;
  http.addHeader("Content-Type","application/json");
  http.addHeader("X-API-Key", API_KEY);
  http.addHeader("X-Device-Id", WiFi.macAddress());
  int code = http.POST(body);
  if(code>0){ out = http.getString(); }
  if(codeOut) *codeOut = code;
  http.end();
  return code>=200 && code<300;
}

// Parse small JSON fields (very naive) for demo
String jsonGet(const String& src, const String& key){
  String k="\"" + key + "\":";
  int i=src.indexOf(k);
  if(i<0) return "";
  i += k.length();
  while(i<src.length() && (src[i]==' '||src[i]=='\"')){ if(src[i]=='\"'){ // string
      int j = src.indexOf("\"", i+1);
      if(j>i) return src.substring(i+1, j);
    }
    i++;
  }
  // number/bool fallback
  int j=i;
  while(j<src.length() && String(",} \n\r\t").indexOf(src[j])==-1) j++;
  return src.substring(i, j);
}

// -------- Flow --------
String terminalId, sessionId, checkoutUrl;

void setup(){
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  Serial.begin(115200);
  prefs.begin("nfc-pay", false);

  wifiSsid = prefs.getString("ssid", "");
  wifiPass = prefs.getString("pass", "");

  // NFC init
  pinMode(PN532_RST, OUTPUT);
  digitalWrite(PN532_RST, LOW); delay(10); digitalWrite(PN532_RST, HIGH); delay(10);
  nfc.begin();
  uint32_t ver = nfc.getFirmwareVersion();
  if (!ver) Serial.println("[ERR] PN532 not found");

  if(wifiSsid.length()<1){
    startAP(); // SoftAP for first setup
  } else {
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifiSsid.c_str(), wifiPass.c_str());
    Serial.printf("[WiFi] Connecting to %s ...\n", wifiSsid.c_str());
  }
}

unsigned long lastPoll=0;
void loop(){
  if(apMode){
    ledPatternError();
    server.handleClient();
    delay(10);
    return;
  }

  if(WiFi.status() != WL_CONNECTED){
    ledPatternError();
    static unsigned long last=0;
    if(millis()-last>12000){
      last = millis();
      Serial.println("[WiFi] still connecting..");
    }
    return;
  }

  // Bootstrap once
  if(terminalId.length()==0){
    String resp;
    if(httpsGet("/api/v1/bootstrap", resp)){
      terminalId = jsonGet(resp, "terminalId");
      checkoutUrl = jsonGet(resp, "checkoutUrl");
      Serial.printf("[BOOTSTRAP] terminal=%s url=%s\n", terminalId.c_str(), checkoutUrl.c_str());
    }
  }

  // Create session once
  if(sessionId.length()==0 && terminalId.length()>0){
    String body = "{}"; // server uses device defaults (amount/currency)
    String resp; int code=0;
    if(httpsPostJson("/api/v1/sessions", body, resp, &code)){
      sessionId = jsonGet(resp, "id");
      Serial.printf("[SESSION] id=%s\n", sessionId.c_str());
      // Write NFC
      if(checkoutUrl.length()>0){
        if(writeNdefUrl(checkoutUrl)){
          Serial.println("[NFC] URL written");
          buzz(60);
        } else {
          Serial.println("[NFC] write failed");
        }
      }
    } else {
      Serial.printf("[SESSION] create failed (%d)\n", code);
      delay(1500);
      return;
    }
  }

  // Poll status
  if(sessionId.length()>0 && millis()-lastPoll>1500){
    lastPoll = millis();
    String resp;
    if(httpsGet(String("/api/v1/sessions/") + sessionId, resp)){
      String status = jsonGet(resp, "status");
      if(status.indexOf("paid")>=0){
        ledPatternPaid();
        buzz(100); delay(120); buzz(100); delay(120); buzz(100);
      } else {
        ledPatternPending();
      }
    } else {
      ledPatternError();
    }
  }
}
