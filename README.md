# ESP32 Reflow Oven Controller (Frankenstein Build)

## Overview
This firmware targets an ESP32-based reflow oven controller with a TFT UI, physical front‑panel buttons, Wi‑Fi web status, PID temperature control, and PWM‑style SSR drive (no phase‑angle/zero‑cross logic in this build).

Key points from the current sketch:
- Thermocouple readout via **MAX6675** (SPI, 16‑bit frame).
- Heater output driven by **LEDC PWM** on the SSR control pin.
- Front‑panel buttons labeled **Warmer/Grill/Toast/Bake/Clock** mapped to UI navigation; **Clock** acts as an **E‑Stop** (hard error state).
- Wi‑Fi web endpoints: `/status`, `/start`, `/stop`.

## Build & Upload
- Open `ReflowController/ReflowController.ino` in the Arduino IDE.
- Board: ESP32 (WROOM‑32 class). Select the correct serial port.
- Libraries are vendored under `ReflowController/src/`.
- Compile with `Sketch > Verify`, then upload with `Sketch > Upload`.

## Pinout (from `ReflowController.ino`)

| Function | GPIO | Details |
| --- | --- | --- |
| TFT LCD CS | 15 | SPI chip‑select |
| TFT LCD DC | 27 | Data/command |
| TFT LCD RESET | 33 | Shared with encoder button |
| SD CS | 5 | Shared with “Toast” button + SCR PWM |
| Encoder A | 35 | Input‑only pin |
| Encoder B | 32 | Shared with thermocouple CS |
| Encoder Button | 33 | Shared with LCD reset |
| Button Start/Stop | 4 | Shared with zero‑cross input definition (not used) |
| Button “Warmer” (label) | 16 | Function: menu up |
| Button “Grill” (label) | 17 | Function: menu down |
| Button “Toast” (label) | 5 | Function: menu left; shared with SD CS/SCR PWM |
| Button “Bake” (label) | 18 | Function: menu right |
| Button “Clock” (label) / E‑Stop | 19 | Function: menu back; triggers `reportError("E-Stop")`; shared with NTC ADC |
| Button ADC (multi‑key) | 36 / ADC1_0 | Analog ladder ranges: ~0–60, 600–700, 1000–1300, 1500–1700, open=4095 (not used in code) |
| Heater SSR | 22 | LEDC PWM output |
| Zero‑Cross Input | 4 | Defined but not used in current control scheme |
| Fan Output | 21 | Digital output |
| Thermocouple CS | 32 | MAX6675 chip‑select; shared with encoder B |
| NTC ADC | 19 | Commented as ADC1_3 in sketch |
| Buzzer | 2 | Tone output |
| SCR PWM Output | 5 | LEDC PWM (shared) |

Notes:
- Several pins are shared/multiplexed. Resolve conflicts if you wire all features.
- GPIOs 34–39 are input‑only (used for the encoder A and ADC ladder).

## Safety
This project drives mains voltage. Use proper isolation, fusing, and enclosure practices. Do not operate unattended.

## License
MIT (see source header and original project attribution in code).
