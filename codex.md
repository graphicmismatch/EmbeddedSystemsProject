# HealthPet Codex Tracker

## Assumptions

- Board target: Seeed Studio XIAO SAMD21.
- OLED: 128x64 SSD1306 over I2C at `0x3C` as defined in the existing sketch.
- RTC module assumption: DS3231 on the same I2C bus as the OLED.
- Vibration sensor assumption: SW-420 module used as a digital activity pulse source.
- Buzzer assumption: passive piezo buzzer so short melodies can be generated with `tone()`.

## Done

- Inspected the existing firmware and preserved the OLED setup details already present in code.
- Confirmed the current project is a minimal splash-screen sketch and needs full game logic, input handling, and animation infrastructure.
- Collected XIAO SAMD21 memory and pin mapping references to keep the animation system within realistic hardware limits.
- Added the full firmware loop with RTC-based daily rollover, 3-day rolling activity average, gentle target growth, health/affection rules, inventory rewards, three-button interaction, and buzzer cues.
- Moved the SW-420 activity path to an interrupt-driven pulse counter and added targeted inline ARM assembly for atomic ISR snapshots and `WFI` idle sleep on the SAMD21.
- Added placeholder sprite symbols in `Images.h` that match the required asset naming convention.
- Added `WIRING.md` with a full pin map and text wiring diagram.
- Updated `img2bitmap.py` so transparent PNG sprites convert cleanly into the bitmap format used by the sketch.
- Added `SPRITES.md` listing the required sprite files for all health tiers and pet states.

## Remaining

- Replace the placeholder bitmaps in `Images.h` with real converted sprite assets once art is ready.
- Verify the firmware against the exact Arduino board package and installed libraries on the target machine.
- Tune sensor threshold and activity balancing using real hardware movement data.
