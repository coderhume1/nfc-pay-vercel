# ESP32 Writer (WS2812 + Buzzer + SoftAP Config + Factory Reset)

## SoftAP Config Portal
- Hold **GPIO0 (BOOT)** while powering on → device starts AP mode:
  - SSID: `NFC-PAY-SETUP-XXYY`
  - Password: *(empty by default; set in `config.h`)*
  - Open `http://192.168.4.1/` → fill **Wi-Fi**, **BASE_URL**, **DEVICE_KEY** → Save & Reboot
- The device also auto-enters Config Portal if Wi‑Fi fails or if `BASE_URL`/`DEVICE_KEY` is missing.

## Factory Reset
- Hold the **CONFIG_BUTTON_PIN** (GPIO0) **LOW for 5s** during power‑up to wipe settings (NVS) and reboot.
- Or browse to `http://192.168.4.1/reset` in Config Portal.

## LED + Buzzer Feedback
- Boot: teal double blink + short beep
- Wi-Fi OK: green blips + short beep
- Config Portal: yellow triple
- PN532 missing: magenta triple + beeps
- Idle: dim blue
- New job: amber double + 2 beeps
- Success: green triple + long beep
- Error: red triple + triple beeps

## Pins (default)
- WS2812B data: **GPIO 4** (`LED_PIN`), `LED_COUNT=8`
- Buzzer: **GPIO 15** (`BUZZER_PIN`)
  - `BUZZER_IS_PASSIVE=1` for passive buzzer (tone via PWM), set to `0` for active buzzer.
  - Use `BUZZER_ACTIVE_HIGH` to match your wiring.

## Runtime Config
Settings are stored in NVS. Defaults are in `src/config.h`:
```cpp
#define DEFAULT_WIFI_SSID "YOUR_WIFI"
#define DEFAULT_WIFI_PASS "YOUR_PASS"
#define DEFAULT_BASE_URL  "https://your-app.vercel.app"
#define DEFAULT_DEVICE_KEY "dev-device-key"
```
Change defaults and perform **Factory Reset** to apply them.

## Build
Open `firmware/device-writer/` in PlatformIO, edit `src/config.h`, upload, and monitor at **115200** baud.
