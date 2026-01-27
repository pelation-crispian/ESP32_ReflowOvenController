# Changelog

## Unreleased
### Thermocouple & Sensor Handling
- Switched thermocouple read logic to 16-bit MAX6675-style parsing (2-byte SPI read, 0.25°C LSB).
- Updated error/status bit handling for the new thermocouple frame.
- Temperature sampling interval set to 250 ms with a single-sample average.

### Heater Control & Zero-Crossing
- Added SCR PWM control path and related LEDC configuration.
- Zero-cross/phase control code removed in favor of PWM-style output.
- SSR output now driven via PWM rather than phase-angle timing.

### Hardware & IO Mapping
- Updated GPIO assignments for TFT, heater, fan, buzzer, and buttons.
- Added multiple front-panel buttons and ADC input for key scanning.
- Added fan output pin and related control wiring.

### UI & Input Handling
- Added button handling utilities and timing helpers for UI actions.
- Menu visibility increased to show more items per page.

### Dependencies
- Added MAX6675 thermocouple library include.
- Added button handling and timing helper libraries.
