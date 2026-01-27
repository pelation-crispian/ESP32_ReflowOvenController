# Repository Guidelines

## Project Structure & Module Organization
- `ReflowController/ReflowController.ino` is the main Arduino sketch (ESP32 firmware).
- `ReflowController/root_html.h` contains the embedded web UI assets served by the device.
- `ReflowController/src/` vendors required Arduino libraries (Adafruit GFX/ST7735, PID, Menu, ClickEncoder).
- `images/` and `Schamatic_ESP8266.png` hold build photos and schematic imagery referenced by the README.

## Build, Test, and Development Commands
- **Arduino IDE (recommended version 1.8.9):** open `ReflowController/ReflowController.ino`, then use `Sketch > Verify` to compile and `Sketch > Upload` to flash.
- **Board selection:** in Arduino IDE, select the ESP32 board under `Tools > Board` and the correct serial port under `Tools > Port`.

## Coding Style & Naming Conventions
- C++/Arduino style with 2‑space indentation as seen in `ReflowController/ReflowController.ino`.
- Constants use `#define` with ALL_CAPS (e.g., `READ_TEMP_INTERVAL_MS`).
- Types use `*_t` or `*_s` suffixes (e.g., `Profile_t`, `profileValues_s`).
- Prefer small, focused functions and keep UI/menu code grouped near related handlers.

## Testing Guidelines
- No automated test framework is included. Validate changes by compiling in the Arduino IDE and doing a smoke test on hardware (encoder input, TFT output, heater relay switching, and web UI render).
- If you add tests, document how to run them here.

## Commit & Pull Request Guidelines
- Existing history uses short, imperative messages (e.g., “Update README.md”, “resize images”). Keep commits concise and scoped.
- PRs should describe hardware used (ESP32 board, display, thermocouple), include before/after photos or screenshots for UI changes, and note any safety‑critical behavior changes.

## Safety & Configuration Notes
- This project controls mains voltage. Include clear warnings in PRs that affect power control logic.
- Avoid committing local Wi‑Fi credentials; keep device configuration on hardware only.
