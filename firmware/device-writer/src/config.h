// firmware/device-writer/src/config.h
#pragma once

// ==== WIFI ====
#define WIFI_SSID "YOUR_WIFI"
#define WIFI_PASS "YOUR_PASS"

// ==== BACKEND ====
#define BASE_URL "https://your-app.vercel.app"
#define DEVICE_KEY "dev-device-key"

// ==== WS2812B ====
#define LED_PIN 4          // Data pin to WS2812B strip
#define LED_COUNT 8        // Number of LEDs
#define LED_BRIGHTNESS 40  // 0..255

// ==== BUZZER ====
// If you have a PASSIVE buzzer, tones will be generated via PWM.
// If you have an ACTIVE buzzer (already oscillates), we'll just turn the pin on/off.
#define BUZZER_PIN 15
#define BUZZER_IS_PASSIVE 1   // 1=passive (use tone), 0=active (on/off)
#define BUZZER_ACTIVE_HIGH 1  // set to 0 if your buzzer is active-low

// Polling interval (ms)
#define POLL_MS 1000
