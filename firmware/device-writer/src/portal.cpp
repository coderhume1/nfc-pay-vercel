// firmware/device-writer/src/portal.cpp
#include "portal.h"
#include "config.h"

#include <WiFi.h>
#include <WebServer.h>

static WebServer server(80);

static const char* htmlForm = R"HTML(
<!doctype html><html><head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>NFC-Pay Writer Setup</title>
<style>
body{font-family:system-ui,Arial;margin:20px;max-width:720px}
label{display:block;margin-top:12px;font-weight:600}
input{width:100%;padding:10px;border:1px solid #ccc;border-radius:8px}
button{margin-top:16px;padding:12px 18px;border:0;border-radius:10px;background:#0a7cff;color:#fff;font-weight:700}
code{background:#f5f5f5;padding:2px 6px;border-radius:6px}
.card{padding:16px;border:1px solid #e5e5e5;border-radius:12px;background:#fff}
.small{color:#666;font-size:12px}
</style></head><body>
<h2>ESP32 NFC-Pay Writer – Setup</h2>
<div class="card">
<form method="POST" action="/save">
<label>Wi-Fi SSID</label>
<input name="wifiSsid" placeholder="WiFi name" value="%WIFI_SSID%">
<label>Wi-Fi Password</label>
<input name="wifiPass" placeholder="WiFi password" value="%WIFI_PASS%">
<label>BASE_URL</label>
<input name="baseUrl" placeholder="https://your-app.vercel.app" value="%BASE_URL%">
<label>DEVICE_KEY</label>
<input name="deviceKey" placeholder="unique-writer-key" value="%DEVICE_KEY%">
<button type="submit">Save & Reboot</button>
</form>
<p class="small">After reboot, the device will join the Wi‑Fi and connect to <code>/api/v1/bootstrap</code>.</p>
</div>
<p><a href="/reset">Factory Reset</a></p>
</body></html>
)HTML";

static String render(const WriterConfig &cfg) {
  String html = htmlForm;
  html.replace("%WIFI_SSID%", cfg.wifiSsid);
  html.replace("%WIFI_PASS%", cfg.wifiPass);
  html.replace("%BASE_URL%",  cfg.baseUrl);
  html.replace("%DEVICE_KEY%",cfg.deviceKey);
  return html;
}

void runConfigPortal(WriterConfig &cfg, uint32_t timeoutMs) {
  // Build SSID with suffix from MAC
  uint8_t mac[6]; WiFi.softAPmacAddress(mac);
  char ssid[64];
  snprintf(ssid, sizeof(ssid), "%s-%02X%02X", SOFTAP_SSID_PREFIX, mac[4], mac[5]);

  if (strlen(SOFTAP_PASSWORD) > 0) {
    WiFi.softAP(ssid, SOFTAP_PASSWORD);
  } else {
    WiFi.softAP(ssid);
  }
  IPAddress ip = WiFi.softAPIP();
  Serial.printf("[AP] %s started, IP: %s\n", ssid, ip.toString().c_str());

  server.on("/", HTTP_GET, [&](){
    server.send(200, "text/html", render(cfg));
  });
  server.on("/save", HTTP_POST, [&](){
    if (server.hasArg("wifiSsid")) cfg.wifiSsid = server.arg("wifiSsid");
    if (server.hasArg("wifiPass")) cfg.wifiPass = server.arg("wifiPass");
    if (server.hasArg("baseUrl"))  cfg.baseUrl  = server.arg("baseUrl");
    if (server.hasArg("deviceKey"))cfg.deviceKey= server.arg("deviceKey");
    saveConfig(cfg);
    server.send(200, "text/html", "<meta http-equiv='refresh' content='2;url=/'/><h3>Saved. Rebooting…</h3>");
    delay(600);
    ESP.restart();
  });
  server.on("/reset", HTTP_GET, [&](){
    clearConfig();
    server.send(200, "text/html", "<h3>Factory reset done. Rebooting…</h3>");
    delay(600);
    ESP.restart();
  });
  server.onNotFound([&](){ server.send(404, "text/plain", "Not found"); });

  server.begin();
  uint32_t start = millis();
  while (true) {
    server.handleClient();
    delay(5);
    if (timeoutMs && millis() - start > timeoutMs) break;
  }
}
