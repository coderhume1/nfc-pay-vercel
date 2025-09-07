#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <SPI.h>
#include <Adafruit_PN532.h>

// -------- Pins --------
#define FACTORY_PIN 34  // tie to GND on boot for factory reset (use external pull-up to 3.3V)
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
String wifiSsid, wifiPass, storeCode;

// -------- SoftAP --------
WebServer server(80);
bool apMode = false;


String htmlIndex() {
  String s = "<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'/>"
             "<style>body{font-family:system-ui,-apple-system,Segoe UI,Roboto,Ubuntu,'Helvetica Neue',Arial,sans-serif;background:#f6f7fb;color:#111;padding:24px;margin:0;}"
             ".card{max-width:720px;margin:0 auto;background:#fff;border-radius:14px;box-shadow:0 10px 30px rgba(20,20,50,.08);padding:20px;}"
             "h1{font-size:20px;margin:0 0 10px}h2{font-size:16px;margin:16px 0 8px;color:#374151}"
             "label{display:block;font-size:12px;color:#6b7280;margin:12px 0 6px}"
             "input,select,button{width:100%;box-sizing:border-box;border:1px solid #d1d5db;border-radius:10px;padding:10px 12px;font-size:14px;outline:none;}"
             "input:focus,select:focus{border-color:#2563eb;box-shadow:0 0 0 3px rgba(37,99,235,.15);}"
             ".row{display:grid;grid-template-columns:1fr 1fr;gap:12px}"
             ".btn{background:#2563eb;color:#fff;border:none;cursor:pointer}"
             ".btn:disabled{opacity:.7;cursor:not-allowed}"
             ".muted{color:#6b7280;font-size:12px}"
             ".top{display:flex;align-items:center;justify-content:space-between;margin-bottom:8px}"
             ".pill{display:inline-flex;align-items:center;gap:8px;border:1px solid #e5e7eb;border-radius:999px;padding:6px 10px;background:#f9fafb;font-size:12px;color:#374151}"
             ".right{display:flex;gap:8px}"
             ".grid{display:grid;gap:12px}"
             ".wifi-row{display:flex;align-items:center;justify-content:space-between;gap:8px}"
             ".bars{display:inline-block;width:18px;height:12px;background:linear-gradient(90deg,#e5e7eb 20%,transparent 20% 40%,#e5e7eb 40% 60%,transparent 60% 80%,#e5e7eb 80%);border-radius:3px}"
             ".ok{color:#059669} .warn{color:#f59e0b} .err{color:#dc2626}"
             ".hr{height:1px;background:#e5e7eb;margin:16px 0}"
             ".actions{display:flex;gap:8px;flex-wrap:wrap}"
             ".inline{display:flex;gap:8px;align-items:center}"
             "</style></head><body>";
  s += "<div class='card'>";
  s += "<div class='top'><h1>NFC‑PAY Wi‑Fi Setup</h1><span class='pill'>Device: ";
  s += WiFi.macAddress();
  s += "</span></div>";
  s += "<p class='muted'>If your phone warns there’s no Internet, stay connected. Use <b>http://192.168.4.1</b> (not https).</p>";
  s += "<div class='hr'></div>";
  s += "<form method='POST' action='/save' class='grid'>";
  s += "<div><label>Wi‑Fi network</label><select id='ssidSelect'><option value=''>— Select network —</option></select></div>";
  s += "<div class='inline'><input id='ssidInput' name='ssid' placeholder='Or type SSID manually'/>";
  s += "<button type='button' class='btn' id='refreshBtn'>Refresh</button></div>";
  s += "<div class='row'><div><label>Password</label><div class='inline'><input id='passInput' name='pass' type='password' placeholder='Wi‑Fi password'/><button type='button' id='showBtn'>Show</button></div></div>";
  s += "<div><label>Store Code (optional)</label><input id='storeInput' name='store' placeholder='e.g. STORE01'/></div></div>";
  s += "<div class='actions'><button class='btn' type='submit'>Save & Reboot</button><button formaction='/factory-reset' formmethod='POST' class='btn' style='background:#ef4444'>Factory Reset</button></div>";
  s += "<p class='muted'>Factory reset also available via hardware: hold <b>GPIO34</b> LOW for 3s at boot.</p>";
  s += "</form>";
  s += "</div>";
  s += "<script>\
  const sel = document.getElementById('ssidSelect');\
  const inSsid = document.getElementById('ssidInput');\
  const inPass = document.getElementById('passInput');\
  const btnRefresh = document.getElementById('refreshBtn');\
  const btnShow = document.getElementById('showBtn');\
  btnShow.addEventListener('click', ()=>{ inPass.type = (inPass.type==='password'?'text':'password'); });\
  sel.addEventListener('change', ()=>{ inSsid.value = sel.value; });\
  async function load(){\
    btnRefresh.disabled = true;\
    try{\
      const r = await fetch('/scan');\
      const j = await r.json();\
      sel.innerHTML = '<option value=\"\">— Select network —</option>';\
      j.aps.forEach(ap=>{\
        const opt = document.createElement('option');\
        opt.value = ap.ssid; opt.textContent = ap.ssid + '  ('+ ap.rssi +' dBm' + (ap.secure ? ', 🔒' : ', 🔓') + ')';\
        sel.appendChild(opt);\
      });\
    }catch(e){}\
    btnRefresh.disabled = false;\
  }\
  btnRefresh.addEventListener('click', load);\
  load();\
  </script>";
  s += "</body></html>";
  return s;
}
input,button{padding:8px;margin:4px;width:100%;max-width:400px}"
             "label{display:block;margin-top:8px}</style></head><body>";
  s += "<h2>NFC-PAY Wi-Fi Setup</h2><p>Device: "' + WiFi.macAddress() + '" — Factory reset: hold pin 34 LOW for 3s at boot.</p>";
  s += "<form method='POST' action='/save'>";
  s += "<label>SSID</label><input name='ssid'/>";
  s += "<label>Password</label><input name='pass' type='password'/><label>Store Code (optional, e.g. STORE01)</label><input name='store'/>";
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
    if(server.hasArg("store")) prefs.putString("store", server.arg("store"));
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

void handleScan(){
  int n = WiFi.scanNetworks(/*async=*/false, /*hidden=*/false);
  String out = "{\"aps\":[";
  for(int i=0;i<n;i++){
    if(i) out += ",";
    String ssid = WiFi.SSID(i);
    long rssi = WiFi.RSSI(i);
    bool secure = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
    // escape quotes in SSID
    ssid.replace("\"","\\\"");
    out += "{\"ssid\":\"" + ssid + "\",\"rssi\":" + String(rssi) + ",\"secure\":" + (secure ? "true" : "false") + "}";
  }
  out += "]}";
  server.sendHeader("Cache-Control","no-store");
  server.send(200, "application/json", out);
}


void startAP(){
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  String ssid = "NFC-PAY-Setup-" + mac.substring(mac.length()-4);
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ssid.c_str(), AP_PASS);
  IPAddress ip = WiFi.softAPIP();
  server.on("/", handleIndex);
  server.on("/scan", handleScan);
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
  if(storeCode.length()>0) http.addHeader("X-Store-Code", storeCode);
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
  storeCode = prefs.getString("store", "");

  
// Factory reset button (GPIO34 input-only; needs external pull-up)
pinMode(FACTORY_PIN, INPUT);
unsigned long t0 = millis();
bool lowDetected = digitalRead(FACTORY_PIN)==LOW;
while(lowDetected && millis()-t0 < 3000) { delay(10); lowDetected = digitalRead(FACTORY_PIN)==LOW; }
if(lowDetected){
  Serial.println("[FACTORY] Triggered. Clearing saved Wi-Fi & store and rebooting...");
  prefs.clear();
  delay(300);
  ESP.restart();
}

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
