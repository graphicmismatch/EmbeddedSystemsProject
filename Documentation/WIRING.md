# Wiring Diagram

Board target: Seeed Studio XIAO SAMD21.

The firmware is structured to fit the XIAO SAMD21 comfortably by keeping sprite frames in flash and avoiding extra frame buffers beyond the SSD1306 display buffer. It assumes the board uses `D4` for `SDA` and `D5` for `SCL`.

## Pin Map

| XIAO SAMD21 Pin | Connects To | Notes |
| --- | --- | --- |
| `3V3` | OLED `VCC`, RTC `VCC`, SW-420 `VCC` | Keep the SW-420 module at `3.3V` so its digital output stays logic-safe for the SAMD21. A bare R-0784 switch does not need `3V3`. |
| `GND` | OLED `GND`, RTC `GND`, SW-420 `GND`, buzzer `-`, all button ground legs | Common ground for every module. Connect one leg of a bare R-0784 sensor here. |
| `D4` | OLED `SDA`, RTC `SDA` | Shared I2C data line. |
| `D5` | OLED `SCL`, RTC `SCL` | Shared I2C clock line. |
| `D0` | Buzzer `+` | Passive piezo buzzer recommended. |
| `D1` | SW-420 `DO` or bare R-0784 other leg | Activity pulse input from the vibration sensor. Firmware now enables `INPUT_PULLUP`, so a bare switch-style vibration sensor can be wired between `D1` and `GND`. |
| `D2` | Left / Prev button | Wire button between pin and ground; firmware uses `INPUT_PULLUP`. |
| `D3` | Center / Action button | Wire button between pin and ground; firmware uses `INPUT_PULLUP`. |
| `D6` | Right / Next button | Wire button between pin and ground; firmware uses `INPUT_PULLUP`. |

## Text Diagram

```text
                Seeed Studio XIAO SAMD21
             +---------------------------+
   3V3  -----+ OLED VCC
           --+ RTC  VCC
           --+ SW420 VCC
   GND  -----+ OLED GND
           --+ RTC  GND
           --+ SW420 GND
           --+ Buzzer -
           --+ Button 1 leg
           --+ Button 2 leg
           --+ Button 3 leg

   D4   -----+ OLED SDA
           --+ RTC  SDA

   D5   -----+ OLED SCL
           --+ RTC  SCL

   D0   -----+ Buzzer +
   D1   -----+ SW420 DO
   D2   -----+ Prev button other leg
   D3   -----+ Action button other leg
   D6   -----+ Next button other leg
             +---------------------------+
```

## Build Notes

- The OLED is assumed to be the existing `128x64` SSD1306 module at I2C address `0x3C`, matching the original sketch.
- The RTC clock is assumed to be a `DS1307`-compatible device at I2C address `0x68`. A `DS3231` module that exposes the same register map for timekeeping will also work for this firmware path.
- Game saves are stored in the TinyRTC module's EEPROM at I2C addresses `0x50`-`0x57` rather than in the RTC clock registers, so save/load can still work even if the RTC clock has been reset.
- On boot, the firmware compares the saved in-game time with the RTC clock. If the RTC advanced while power was off, the pet catches up by that elapsed time. If the RTC clock is missing or reset, the firmware falls back to the saved in-game time, reseeds the RTC when possible, and shows a status warning.
- Adjust the SW-420 module potentiometer so ordinary desk vibration does not spam the activity count.
- If you are using a bare R-0784 sensor instead of a module, wire it between `D1` and `GND`; the internal pull-up in firmware keeps the input stable.
