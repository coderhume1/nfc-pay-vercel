// firmware/device-writer/src/config_store.cpp
#include "config_store.h"
#include <Preferences.h>

static const char* NS = "nfcwriter";

bool loadConfig(WriterConfig &out) {
  Preferences p;
  if (!p.begin(NS, true)) return false;
  out.wifiSsid = p.getString("wifiSsid", "");
  out.wifiPass = p.getString("wifiPass", "");
  out.baseUrl  = p.getString("baseUrl", "");
  out.deviceKey= p.getString("deviceKey", "");
  p.end();
  return out.baseUrl.length() > 0 && out.deviceKey.length() > 0;
}

bool saveConfig(const WriterConfig &cfg) {
  Preferences p;
  if (!p.begin(NS, false)) return false;
  p.putString("wifiSsid", cfg.wifiSsid);
  p.putString("wifiPass", cfg.wifiPass);
  p.putString("baseUrl",  cfg.baseUrl);
  p.putString("deviceKey",cfg.deviceKey);
  p.end();
  return true;
}

void clearConfig() {
  Preferences p;
  if (p.begin(NS, false)) {
    p.clear();
    p.end();
  }
}
