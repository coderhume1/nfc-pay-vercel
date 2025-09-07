# ESP32 Writer (WS2812 + Buzzer)

**Pins (default in `src/config.h`):**
- WS2812B data: `GPIO 4` (8 LEDs by default)
- Buzzer: `GPIO 15`
  - `BUZZER_IS_PASSIVE=1` → uses PWM tone on ESP32 (passive buzzer)
  - `BUZZER_IS_PASSIVE=0` → simple ON/OFF for active buzzer
  - Toggle `BUZZER_ACTIVE_HIGH` if your buzzer is active-low

**Feedback:**
- Boot: teal blinks + short beep
- WiFi OK: green blips + short beep
- No PN532 found: magenta triple blink + beeps
- Idle: dim blue
- New job: amber double blink + 2 beeps
- Success: green triple + long beep
- Error: red triple + triple beeps
