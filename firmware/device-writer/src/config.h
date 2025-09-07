// firmware/device-writer/src/config.h
#pragma once

// ====== RUNTIME-CONFIG (stored in NVS Preferences) ======
// The following defaults are only used if NVS is empty.
// You can change them here, then do a Factory Reset to reapply.
#define DEFAULT_WIFI_SSID "YOUR_WIFI"
#define DEFAULT_WIFI_PASS "YOUR_PASS"
#define DEFAULT_BASE_URL  "https://your-app.vercel.app"
#define DEFAULT_DEVICE_KEY "dev-device-key"

// ====== SOFTAP / CONFIG-PORTAL ======
#define SOFTAP_SSID_PREFIX "NFC-PAY-SETUP"
#define SOFTAP_PASSWORD ""          // empty = open AP; set a password if you prefer
#define CONFIG_BUTTON_PIN 0         // hold LOW at boot to force Config mode (GPIO0/BOOT on many boards)
#define FACTORY_HOLD_MS  5000       // hold button this long at boot to factory reset

// ====== WS2812B ======
#define LED_PIN 4
#define LED_COUNT 8
#define LED_BRIGHTNESS 40

// ====== BUZZER ======
#define BUZZER_PIN 15
#define BUZZER_IS_PASSIVE 1
#define BUZZER_ACTIVE_HIGH 1

// Polling interval (ms)
#define POLL_MS 1000
