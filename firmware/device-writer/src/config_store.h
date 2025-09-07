// firmware/device-writer/src/config_store.h
#pragma once
#include <Arduino.h>

struct WriterConfig {
  String wifiSsid;
  String wifiPass;
  String baseUrl;
  String deviceKey;
};

bool loadConfig(WriterConfig &out);
bool saveConfig(const WriterConfig &cfg);
void clearConfig();
