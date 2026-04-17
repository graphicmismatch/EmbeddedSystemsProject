# HealthPet Project Architecture

## Purpose

`HealthPet` is a small virtual-pet firmware for the Seeed Studio XIAO SAMD21. It runs on a 128x64 SSD1306 OLED, uses a vibration sensor as an activity input, tracks a pet's health and affection over time, stores game state in the TinyRTC EEPROM, and uses an RTC clock to advance in-game time while power is off.

The main sketch is [HealthPet.ino](./HealthPet.ino). The rest of the project supports rendering bitmaps, defining sprite assets, documenting wiring, and converting art into the bitmap format used by the firmware.

## High-Level Architecture

The project has six main runtime subsystems:

1. **Hardware abstraction**
   - OLED display via `Adafruit_SSD1306`
   - I2C RTC clock plus EEPROM via `Wire`
   - buzzer via `tone()` / `noTone()`
   - vibration sensor via GPIO interrupt
   - three buttons via `INPUT_PULLUP`

2. **Persistent state and timekeeping**
   - serializes `PetState` into `SaveData`
   - stores saves in TinyRTC EEPROM
   - mirrors game time to the RTC clock
   - replays elapsed offline time on boot

3. **Pet simulation**
   - tracks health, affection, inventory, activity, and recent history
   - computes daily target and progress
   - applies inactivity, day rollover, and pace-based penalties

4. **Input and activity processing**
   - button navigation and actions
   - sensor interrupt captures raw vibration transitions
   - foreground code converts pulses into activity points and rewards

5. **Presentation**
   - chooses the active animation clip
   - draws UI screens and indicators
   - shows transient status messages

6. **Audio**
   - queued multi-step buzzer patterns for button clicks, rewards, warnings, death, and leaving

## File Map

### Firmware files

- [HealthPet.ino](./HealthPet.ino)
  - entire game firmware
  - data structures, hardware setup, simulation logic, persistence, UI, main loop

- [GraphicsUtils.h](./GraphicsUtils.h)
  - declarations for bitmap drawing helpers and simple UI drawing primitives

- [GraphicsUtils.cpp](./GraphicsUtils.cpp)
  - implementations of helper drawing routines around `Adafruit_SSD1306`

- [Bitmap.h](./Bitmap.h)
  - defines the `Bitmap` struct used by all sprites and icons

- [Images.h](./Images.h)
  - compiled sprite and icon data stored in program memory

### Documentation and tooling

- [WIRING.md](./WIRING.md)
  - wiring guide and board assumptions

- [SPRITES.md](./SPRITES.md)
  - sprite naming and format expectations

- [img2bitmap.py](./img2bitmap.py)
  - converts PNG input into the `Bitmap` format used in `Images.h`

- [questions.md](./questions.md)
  - disassembly and optimization analysis generated from the real firmware

## Core Data Model

### Enums

- `ScreenMode`
  - current UI screen: home, stats, inventory

- `InventorySelection`
  - which inventory item is selected on the inventory screen

- `HealthTier`
  - low, mid, healthy visual tier

- `SaveReadStatus`
  - result of reading and validating save bytes from EEPROM

### Structs

- `ButtonState`
  - tracks pin, current down state, one-shot `pressed` event, and debounce timing

- `ToneStep`
  - one buzzer note or rest with duration and gap

- `SoundEffect`
  - array of `ToneStep` entries

- `AnimationClip`
  - set of bitmap frames plus frame count and frame duration

- `SpriteTierSet`
  - four clips for a given pet health tier: sleep, awake, happy, surprised

- `SaveData`
  - compact serialized representation of persistent game state

- `PetState`
  - live runtime state for the pet and UI

## Important Global State

- `display`
  - SSD1306 display object

- `pet`
  - main runtime state object

- `currentDayCount`, `currentMinuteOfDay`
  - current game time in day/minute form

- `prevButton`, `actionButton`, `nextButton`
  - debounced button state objects

- `activeSound`, `activeSoundStep`, `nextSoundChangeMs`
  - sound playback state

- `rtcStorageAddress`, `rtcClockAvailable`
  - EEPROM and RTC availability tracking

- `lastClockTickMs`, `lastSaveMs`, `lastDrawMs`
  - scheduling timestamps

- `softwareMinuteAccumulatorMs`
  - accumulates real milliseconds into in-game minutes

- `pendingStepRemainder`
  - leftover sensor pulses that did not yet become a full activity point

- `pendingSensorTransitions`, `pendingSensorActiveEdges`, `pendingSensorLevelHigh`, `lastSensorInterruptUs`
  - interrupt-shared sensor state captured by the ISR and consumed in the foreground

## Runtime Flow

### Startup

`setup()` performs the full boot sequence:

1. starts serial logging
2. configures buzzer, buttons, and sensor pins
3. seeds the random number generator
4. starts I2C
5. attaches the vibration sensor interrupt
6. initializes the OLED
7. probes RTC EEPROM and RTC clock availability
8. initializes scheduler timestamps
9. loads save data and syncs or seeds the RTC
10. publishes the initial status message

### Main loop

`loop()` runs these tasks in order:

1. read current `millis()`
2. advance software game time through `updateClock()`
3. debounce the three buttons
4. consume pending sensor activity through `updateSensor()`
5. process user input through `handleInput()`
6. progress sound playback through `updateSound()`
7. autosave if needed through `saveState(false)`
8. redraw the display at the configured refresh interval
9. sleep with `waitForInterrupt()` until the next interrupt

## Interrupts And Low-Level Routines

### Interrupt service routine

- `onSensorInterrupt()`
  - hardware ISR attached to the vibration sensor pin
  - debounces raw interrupt timing in microseconds
  - samples the sensor pin level
  - increments transition and active-edge counters
  - does not perform expensive game logic directly
  - hands work off to the foreground by updating volatile counters

### Inline ARM routines

- `saveAndDisableInterrupts()`
  - saves the ARM `PRIMASK` interrupt state and disables interrupts
  - used to atomically copy ISR-shared counters in `updateSensor()`

- `restoreInterrupts(uint32_t primask)`
  - restores the previously saved interrupt state

- `waitForInterrupt()`
  - issues ARM `wfi`
  - lets the CPU idle efficiently between loop iterations

### I2C clock profile helpers

- `useStorageI2cClock()`
  - switches I2C to the slower clock used for RTC/EEPROM traffic

- `useDisplayI2cClock()`
  - switches I2C back to the faster clock used for the OLED

## Subsystem Walkthrough

### 1. Calendar, RTC, and save serialization

This subsystem converts between calendar time and game time, reads and writes the RTC clock, reads and writes EEPROM save blocks, and validates saved state.

Functions:

- `bcdToDec(uint8_t value)`
  - converts BCD register values from the DS1307-compatible RTC into decimal

- `decToBcd(uint8_t value)`
  - converts decimal values into BCD before writing the RTC

- `isLeapYear(uint16_t year)`
  - leap-year helper for date conversion

- `daysInMonth(uint16_t year, uint8_t month)`
  - returns the day count for a month, including leap-year February

- `writeUint16LE(uint8_t *buffer, uint8_t &offset, uint16_t value)`
  - appends a 16-bit little-endian value to a save buffer

- `writeUint32LE(uint8_t *buffer, uint8_t &offset, uint32_t value)`
  - appends a 32-bit little-endian value to a save buffer

- `readUint16LE(const uint8_t *buffer, uint8_t &offset)`
  - reads a 16-bit little-endian value from a save buffer

- `readUint32LE(const uint8_t *buffer, uint8_t &offset)`
  - reads a 32-bit little-endian value from a save buffer

- `checksumBytes(const uint8_t *buffer, uint8_t length)`
  - computes the save checksum

- `encodeSaveData(const SaveData &save, uint8_t *buffer)`
  - serializes a `SaveData` struct into raw EEPROM bytes and appends checksum

- `decodeSaveDataDetailed(const uint8_t *buffer, SaveData &save)`
  - validates checksum and deserializes raw EEPROM bytes into `SaveData`

- `saveDataEquals(const SaveData &a, const SaveData &b)`
  - deep equality check used to verify save-write success

- `detectRtcStorageAddress()`
  - scans I2C EEPROM addresses `0x50` through `0x57`
  - stores the detected EEPROM address

- `detectRtcClock()`
  - probes the RTC clock device at `0x68`

- `rtcMinutesFromDateTime(...)`
  - converts year/month/day/hour/minute into total minutes since `RTC_BASE_YEAR`

- `rtcDateTimeFromMinutes(...)`
  - inverse of `rtcMinutesFromDateTime`
  - converts total minutes into RTC calendar fields

- `gameTotalMinutes()`
  - converts current in-game day/minute into total minutes

- `readRtcClockMinutes(uint32_t &totalMinutes)`
  - reads RTC registers over I2C and converts them into total minutes

- `writeRtcClockMinutes(uint32_t totalMinutes)`
  - converts total minutes into calendar fields and writes them to the RTC

- `rtcReadBlock(uint16_t startAddress, uint8_t *buffer, uint8_t length)`
  - chunked EEPROM read helper

- `rtcWriteBlock(uint16_t startAddress, const uint8_t *buffer, uint8_t length)`
  - chunked EEPROM write helper with write-cycle delay

- `readSaveDataDetailed(SaveData &save, uint8_t *buffer)`
  - reads the raw EEPROM save slot and decodes it

- `writeSaveData(const SaveData &save)`
  - encodes and writes a save record to EEPROM

- `validateSaveDataReason(const SaveData &saved)`
  - returns a human-readable validation failure reason, or `nullptr` if valid

- `readSaveDataWithRetry(SaveData &save, bool verbose = true)`
  - retries EEPROM reads and optionally logs failures

- `writeSaveDataWithRetry(const SaveData &save)`
  - retries EEPROM writes

### 2. Sound and status messaging

This subsystem manages temporary UI messages and non-blocking buzzer playback.

Functions:

- `setStatus(const char *message, uint32_t now, uint32_t durationMs = STATUS_MESSAGE_MS)`
  - updates the transient message shown at the bottom of the screen

- `playSound(const SoundEffect &sound)`
  - arms a sound effect for playback

- `updateSound(uint32_t now)`
  - advances the buzzer through each `ToneStep` without blocking the main loop

### 3. Pet state, predicates, and derived metrics

This subsystem answers questions about the pet and computes derived values used across gameplay and UI.

Functions:

- `inventoryFull()`
  - checks whether the combined treat and gift count reached capacity

- `petDead()`
  - checks whether health is zero

- `petLeft()`
  - checks whether affection is zero while the pet is not dead

- `petUnavailable()`
  - true when the pet is dead or has left

- `currentTier()`
  - maps health into low, mid, or healthy sprite tier

- `isSleepingNow()`
  - determines whether the current in-game time is in the sleep window

- `isIdleSleeping(uint32_t now)`
  - determines whether the pet should visually sleep because of inactivity

- `rollingAverage()`
  - computes the average of recent completed-day activity history

- `activityTarget()`
  - computes today's target using recent history plus a growth factor

- `todayProgressPercent()`
  - computes progress toward today's target as a capped percentage

- `awakeMinutesElapsed(uint16_t minuteOfDay)`
  - computes how many awake minutes of the current day have elapsed

- `totalAwakeMinutes()`
  - total awake minutes in a full in-game day

- `clampStat(int value)`
  - clamps health and affection into the allowed `0..100` range

- `markDirty()`
  - marks state as needing a save

### 4. State initialization, load, save, and reset

This subsystem creates fresh state, restores saved state, syncs time, and persists changes.

Functions:

- `setDefaultState()`
  - resets `pet` and related counters to a new-game baseline

- `isValidSaveData(const SaveData &saved)`
  - validation wrapper around `validateSaveDataReason`

- `loadState(uint32_t now)`
  - boots the pet from defaults, attempts EEPROM restore, applies offline RTC catch-up, and sets user-facing status

- `restartPet(uint32_t now)`
  - starts a fresh pet after death or leaving
  - resets day/time counters and state
  - marks state dirty and syncs the RTC

- `saveState(bool force)`
  - performs throttled autosave or forced save
  - writes `SaveData`, reads it back, and verifies the result

### 5. Daily progression and simulation rules

This subsystem is the heart of the pet simulation.

Functions:

- `pushCompletedDay(uint32_t dayActivity)`
  - appends a completed day's activity into fixed-length history

- `triggerHappy(uint32_t now)`
  - starts the happy animation window

- `triggerSurprise(uint32_t now)`
  - starts the surprise animation window

- `completeDay(uint32_t dayActivity, uint32_t now, bool notify = true)`
  - finalizes one day of activity
  - stores the day in history
  - resets day-specific counters
  - emits success or warning feedback based on target completion

- `applyActivityPaceDrain(uint32_t now, bool notify = true)`
  - penalizes health and affection if the pet's activity pace is too low during awake hours
  - also triggers death or leaving statuses and sounds when thresholds are crossed

- `syncRtcClockToGameTime()`
  - writes current in-game time into the RTC when available

- `advanceGameMinute(uint32_t now, bool notify = true)`
  - advances the game by one minute
  - triggers day rollover and hourly pace checks

- `advanceGameMinutes(uint32_t elapsedMinutes, uint32_t now, bool notify)`
  - replays many elapsed in-game minutes, mainly used on boot after RTC catch-up

- `updateClock(uint32_t now)`
  - converts real elapsed milliseconds into in-game minutes
  - advances time and mirrors it to the RTC

### 6. Buttons, inventory, rewards, and activity

This subsystem converts raw inputs into game actions.

Functions:

- `updateButton(ButtonState &button, uint32_t now)`
  - debounces one button and raises a one-shot `pressed` event

- `addReward(bool gift, uint32_t now)`
  - adds a treat or gift to inventory and triggers status/sound feedback

- `maybeDropReward(uint32_t now)`
  - reward drop controller
  - only grants rewards when enough activity has been accumulated and adequate spacing has passed since the previous reward

- `onActivityPulse(uint32_t now, uint32_t pulseCount)`
  - foreground activity processor
  - turns raw sensor pulses into activity points
  - updates lifetime stats
  - triggers happy animation milestones
  - triggers reward opportunities after high-exercise pulse bursts

- `updateSensor(uint32_t now)`
  - atomically copies ISR-updated counters
  - clears them
  - forwards active-edge counts into `onActivityPulse()`

- `cycleScreen(int8_t direction, uint32_t now)`
  - rotates among home, stats, and inventory screens

- `petThePet(uint32_t now)`
  - home-screen action
  - revives with a new pet if unavailable, otherwise applies cooldown and affection gain

- `useInventoryItem(uint32_t now)`
  - inventory action
  - consumes a treat or gift and applies stat changes

- `handleInput(uint32_t now)`
  - central button-action dispatcher
  - left/right change screen or inventory selection
  - action pets the pet, uses inventory, or shows a stats hint depending on screen

### 7. Animation and UI rendering

This subsystem chooses what the pet should look like and draws each screen.

Functions:

- `activeClip(uint32_t now)`
  - picks the correct animation clip based on death, leaving, surprise, happy state, and idle sleep

- `currentFrame(uint32_t now)`
  - chooses the current bitmap frame from the active clip using time-based animation

- `printAge()`
  - prints age as days or weeks

- `drawTopBar()`
  - draws health bar, affection bar, and today's progress percentage

- `drawSaveIndicator(uint32_t now)`
  - draws `!`, `*`, or `S` to indicate save failure, dirty state, or recent successful save

- `drawStatusLine(uint32_t now)`
  - draws transient status text, or the default activity/target and age footer

- `drawHomeScreen(uint32_t now)`
  - top bar + animated pet + footer

- `drawStatsScreen()`
  - top bar + activity totals + target + rolling average + age

- `drawInventoryScreen()`
  - top bar + selectable treat and gift counts + action hint

- `drawUi(uint32_t now)`
  - clears the display, dispatches to the correct screen drawer, draws save indicator, and flushes the buffer to the OLED

### 8. Arduino entry points

- `setup()`
  - hardware initialization and boot orchestration

- `loop()`
  - top-level scheduler for time, input, sensor consumption, audio, save, draw, and idle sleep

## Graphics Helper Module

The helper module in `GraphicsUtils.*` is intentionally small. It wraps recurring display operations so the game code stays focused on layout and state.

Functions in [GraphicsUtils.cpp](./GraphicsUtils.cpp):

- `drawBitmapFlash(Adafruit_SSD1306 &display, int16_t x, int16_t y, const Bitmap *bmp)`
  - draws a bitmap from flash memory at a specific location

- `drawBitmapCentered(Adafruit_SSD1306 &display, const Bitmap *bmp)`
  - centers a bitmap in the full display area

- `drawBitmapCenteredAt(Adafruit_SSD1306 &display, int16_t cx, int16_t cy, const Bitmap *bmp)`
  - centers a bitmap around an arbitrary point

- `drawProgressBar(Adafruit_SSD1306 &display, int16_t x, int16_t y, int16_t width, int16_t height, uint8_t percent)`
  - draws a simple outlined progress bar and fills it according to percentage

- `drawFrame(Adafruit_SSD1306 &display, int16_t x, int16_t y, int16_t w, int16_t h)`
  - draws a rectangle outline

- `clearRect(Adafruit_SSD1306 &display, int16_t x, int16_t y, int16_t w, int16_t h)`
  - clears a rectangular area to black

## Asset Architecture

### `Bitmap.h`

- `Bitmap`
  - tiny struct containing:
    - `data`
    - `width`
    - `height`

### `Images.h`

`Images.h` holds all icon and sprite bitmaps in `PROGMEM`. These are organized conceptually as:

- status icons
  - `icon_health`
  - `icon_affection`
  - `icon_sleep`

- inventory icons
  - `item_treat`

- pet animation frames by health tier and mood
  - healthy: awake, happy, sleep, surprised
  - mid: awake, happy, sleep, surprised
  - low: awake, happy, sleep, surprised

- terminal states
  - `pet_dead`
  - `pet_leave`

In the sketch, these raw bitmaps are grouped into:

- frame arrays such as `HEALTHY_AWAKE_FRAMES`
- `AnimationClip` values for each mood
- `SPRITE_TIERS` lookup table by health tier

## Architecture Notes For New Contributors

### Design choices that matter

- The display is buffered by `Adafruit_SSD1306`, so draw code writes to RAM and only updates the OLED in `drawUi()`.
- The sensor ISR is intentionally minimal. Any nontrivial game logic happens later in `updateSensor()`.
- Save writes are verified by an immediate readback to avoid silently accepting corrupted EEPROM writes.
- The project treats RTC clock state and EEPROM save state separately. Save data lives in EEPROM, while the RTC only stores current time.
- The code supports offline progress by comparing saved game minutes to current RTC minutes on boot.
- Most game rules are minute-based, not frame-based, so the simulation remains stable regardless of display refresh rate.

### Best entry points when learning the code

For a newcomer, the most useful reading order is:

1. `setup()`
2. `loop()`
3. `loadState()`
4. `updateClock()`
5. `updateSensor()` and `onSensorInterrupt()`
6. `handleInput()`
7. `drawUi()`
8. `saveState()`

That order matches the real runtime lifecycle from boot to idle loop.
