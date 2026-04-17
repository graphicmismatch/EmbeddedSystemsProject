# HealthPet: Virtual Fitness Companion on the Seeed Studio XIAO SAMD21

## Comprehensive Technical Report

## Table of Contents

1. Motivation and Background
2. Problem Statement
3. Project Objectives
4. Unique Selling Proposition (USP)
5. Overall System Overview
6. Hardware Description
7. Functional Block-Level Explanation
8. Working Principle
9. Interfacing
10. Networking and Communication Protocols
11. Software Architecture
12. Data Structures and State Model
13. Programming Logic
14. Detailed Functional Blocks Used in the Project
15. Memory Organization and Run-Time Environment
16. Debugging Strategy and Debugging Process
17. Optimization Strategy
18. Final Working Model
19. Measured Technical Results
20. Limitations
21. Future Scope
22. Conclusion
23. References

---

## 1. Motivation and Background

The motivation for this project is grounded in the idea that emotional and interactive systems can encourage more consistent engagement than passive displays. The initial project proposal, contained in `Review1.pdf`, framed the system as a **virtual fitness companion** rather than a conventional pedometer or activity dashboard.

The proposal emphasized several key ideas:

- emotional state can be used as a motivational signal
- personalized activity feedback is more realistic than one fixed goal for all users
- a small low-power microcontroller is sufficient to host an expressive interactive system
- activity reminders can be made more engaging when they are presented as the needs or mood of a companion rather than as impersonal alerts

The proposal also connected the system to behavioral ideas:

- emotionally expressive digital agents can influence user behavior
- both positive and negative feedback may be motivational when applied carefully
- routine care for a companion can help reinforce habit formation

The current implemented project realizes these ideas in an embedded form:

- the pet becomes happier with user activity
- low activity eventually reduces health and affection
- daily targets adapt using recent history instead of a hard-coded universal number
- the device maintains continuity through saved state and RTC synchronization

Thus, the project motivation is both technical and human-centered. Technically, it demonstrates a complete embedded interactive product on resource-constrained hardware. Behaviorally, it attempts to make physical activity feel visible, immediate, and personally meaningful.

---

## 2. Problem Statement

Many embedded wellness devices display raw metrics such as steps, time, or simple bars. While these metrics are useful, they do not always sustain user attention over long periods. Static targets can also become discouraging when they do not reflect the user’s current baseline or lifestyle.

The problem addressed by this project is:

> How can a small embedded system motivate ongoing physical activity using simple hardware, while maintaining low resource usage, continuity of state, and emotionally interpretable feedback?

To solve this problem, the project must meet several requirements simultaneously:

- sense physical activity through a compact sensor path
- provide feedback that is more engaging than numeric data alone
- maintain state between resets and power cycles
- adapt expectations to the user’s recent behavior
- fit within the memory and execution constraints of the SAMD21
- remain robust enough for long-running use

`HealthPet` addresses these requirements by combining embedded firmware architecture with lightweight game-state logic and an emotionally expressive OLED interface.

---

## 3. Project Objectives

The implemented system reflects the following objectives.

### 3.1 Emotional State System

The pet should exhibit different visible states according to user activity and internal condition:

- awake
- sleeping
- happy
- surprised
- weakened or unhealthy
- dead
- left the user

These states are not arbitrary. They convert internal game state into immediately interpretable user feedback.

### 3.2 Personalized Activity Feedback

The project should avoid relying solely on a fixed universal target. Instead, it should compute a personalized activity target based on recent performance.

In the firmware, this is implemented using:

- a rolling activity history
- a minimum baseline
- a small percentage-based target increase

### 3.3 Sensor-Based Activity Tracking

The system should convert physical motion into activity counts and then into pet-state changes. For the implemented version, a vibration sensor is used as the activity signal source.

### 3.4 Persistent Interaction

The pet should not reset its life entirely every time power is removed. The system therefore stores save data externally and uses an RTC clock to preserve elapsed time.

### 3.5 Embedded Efficiency

The system should remain practical on a SAMD21 MCU. This requires:

- bounded memory use
- no application-level dynamic allocation
- short interrupt handlers
- non-blocking scheduling
- compact display assets

---

## 4. Unique Selling Proposition (USP)

The most important USP of `HealthPet` is that it transforms physical activity monitoring into a **persistent relationship-driven embedded interaction** rather than a simple activity counter.

The project’s distinguishing points are:

1. **Emotion-driven embedded feedback**
   - User movement changes the perceived wellbeing of a virtual companion.

2. **Personalized target generation**
   - The device adapts daily activity expectation using recent history instead of a rigid universal threshold.

3. **RTC-backed continuity**
   - The pet does not simply restart each boot; it can recover elapsed time and continue evolving.

4. **Low-resource implementation**
   - The entire system runs on a compact SAMD21 with an OLED, simple sensor input, and external I2C peripherals.

5. **Integrated system design**
   - The project combines sensing, storage, UI, timing, audio, and optimization in one coherent embedded product.

6. **Visible cause-and-effect interaction**
   - A user can directly observe how activity, inactivity, rewards, and neglect affect the pet.

In academic and technical terms, the project is not only a game interface, but also a demonstration of how embedded design can encode behavioral feedback loops using constrained hardware.

---

## 5. Overall System Overview

At a high level, the system contains:

- one microcontroller board
- one OLED display
- one RTC clock
- one EEPROM storage path
- one vibration sensor
- three buttons
- one buzzer
- sprite assets in flash
- a software state machine that links user activity to pet behavior

The system can be viewed in layers:

### Physical layer

- sensor input
- button input
- OLED output
- buzzer output
- I2C peripheral communication

### Embedded control layer

- interrupt capture
- debouncing
- scheduling
- save/load
- clock update

### Application logic layer

- pet health and affection
- rewards and inventory
- daily target logic
- emotional state transitions

### Presentation layer

- animation selection
- text display
- status messages
- progress bars

---

## 6. Hardware Description

The project is designed for the **Seeed Studio XIAO SAMD21**.

### 6.1 Microcontroller Board

Board:

- Seeed Studio XIAO SAMD21

Processor:

- ARM Cortex-M0+

Clock:

- 48 MHz

Memory:

- 256 KB flash
- 32 KB RAM

Why this board is suitable:

- sufficient flash for graphics and firmware logic
- enough RAM for display buffer and runtime state
- low power
- Arduino ecosystem support
- I2C support for OLED and RTC
- adequate GPIO for buttons, sensor, and buzzer

### 6.2 OLED Display

Display type:

- 128x64 SSD1306 OLED with a dual-color panel
- top band appears yellow
- main display area appears blue

Interface:

- I2C

Address:

- `0x3C`

Role:

- displays pet sprites
- shows progress bars for health and affection
- displays inventory, stats, save state, and status messages

Practical display note:

- although the SSD1306 controller is still driven as a 1-bit display in software, the physical OLED panel is dual-color
- in the actual module used for this project, the upper band is yellow while the remaining display area is blue
- this makes the top status bar visually distinct from the rest of the interface

### 6.3 RTC Clock Module

Clock type:

- DS1307-compatible RTC path assumed by firmware

I2C address:

- `0x68`

Role:

- stores current wall-clock-like time for the game
- allows time progression to persist even when the device is powered off

### 6.4 EEPROM Storage

Storage path:

- TinyRTC EEPROM region at I2C addresses `0x50` to `0x57`

Role:

- stores saved game state
- keeps health, affection, inventory, activity history, and in-game time

### 6.5 Vibration Sensor

Input source:

- SW-420 module or switch-style vibration sensor

Pin:

- `D1`

Role:

- converts physical movement into digital pulses
- feeds the activity subsystem

### 6.6 Buttons

Buttons:

- previous
- action
- next

Pins:

- `D2`, `D3`, `D6`

Configuration:

- `INPUT_PULLUP`

Role:

- screen navigation
- item selection
- pet interaction
- inventory usage

### 6.7 Buzzer

Type:

- passive buzzer

Pin:

- `D0`

Role:

- button feedback
- reward cues
- warning sounds
- pet death/leave sounds

### 6.8 Power and Bus Sharing

The OLED, RTC, and EEPROM share the I2C bus. The firmware explicitly changes the I2C clock depending on which device is being accessed:

- storage/RTC traffic at a slower clock
- display traffic at a faster clock

This is a practical example of bus-aware embedded design.

---

## 7. Functional Block-Level Explanation

### 7.1 Sensor Block

Captures motion and generates raw pulses. This is the starting point for activity-based game changes.

### 7.2 Interrupt Block

Converts raw pin changes into debounced transition counters without doing heavy work inside the ISR.

### 7.3 Activity Processing Block

Converts pulses into activity points, happiness triggers, and reward opportunities.

### 7.4 Timekeeping Block

Advances in-game time, detects day rollover, and synchronizes game time with RTC time.

### 7.5 Persistence Block

Serializes and restores the pet state through EEPROM with checksum validation and retry logic.

### 7.6 UI Block

Chooses the current screen and draws dynamic content.

### 7.7 Animation Block

Maps pet condition to sprite clips and chooses the current frame.

### 7.8 Audio Block

Produces short sound sequences that reinforce UI and pet-state events.

---

## 8. Working Principle

The working principle of the project is based on converting physical activity into pet wellbeing over time.

### 8.1 Core Principle

When the user moves, the vibration sensor produces pulses. The firmware interprets those pulses as activity. Activity improves the pet’s effective daily progress and can trigger positive animations and rewards. If the user remains insufficiently active, the pet gradually loses health and affection. In extreme cases, the pet can die or leave.

Thus:

```text
User movement -> sensed activity -> progress increase -> positive pet response
Low activity -> unmet pace -> penalties -> negative pet response
```

### 8.2 Step-by-Step Working Principle

1. The vibration sensor changes state.
2. The pin interrupt fires.
3. The ISR records the event and debounces it.
4. The foreground loop copies the pending event counts safely.
5. The event count is converted into activity gain.
6. The pet state is updated.
7. If enough activity is achieved:
   - the pet becomes happy
   - rewards may be granted
8. Time progresses continuously.
9. If progress remains below expectation:
   - affection and health decrease
10. The display updates to reflect the new state.
11. The save system periodically stores the result.
12. RTC time ensures continuity across resets.

### 8.3 Personalization Principle

The system does not rely only on one fixed target. It computes a target based on:

- a rolling recent activity average
- a minimum floor
- a small growth factor

This creates a target that is challenging but not arbitrary.

### 8.4 Emotional Feedback Principle

The pet’s behavior acts as the interface:

- good activity -> healthy/happy pet
- low activity -> weaker, sadder, or eventually absent pet

This creates emotional continuity and makes the feedback more immediate than plain numerical reporting.

---

## 9. Interfacing

Interfacing in this project refers to both hardware interfacing and software-level peripheral integration.

### 9.1 OLED Interfacing

Interface:

- I2C

Connections:

- `D4` -> SDA
- `D5` -> SCL

Library:

- `Adafruit_SSD1306`
- `Adafruit_GFX`

Software behavior:

- the display is initialized in `setup()`
- the draw buffer is updated in RAM
- `display.display()` flushes the buffer to the OLED

### 9.2 RTC and EEPROM Interfacing

Interface:

- I2C

Clock address:

- `0x68`

EEPROM scan range:

- `0x50` through `0x57`

Software behavior:

- RTC is probed during startup
- EEPROM is scanned until a valid storage address is found
- game time is synchronized with the RTC
- save blocks are read and written in chunks

### 9.3 Sensor Interfacing

Interface:

- GPIO digital input with interrupt

Pin:

- `D1`

Mode:

- `INPUT_PULLUP`

Interrupt mode:

- `CHANGE`

Software behavior:

- each sensor change triggers `onSensorInterrupt()`
- raw events are debounced
- foreground code processes counts later

### 9.4 Button Interfacing

Interface:

- GPIO digital input

Pins:

- `D2`, `D3`, `D6`

Mode:

- `INPUT_PULLUP`

Software behavior:

- polled in `loop()`
- debounced in software
- converted into one-shot press events

### 9.5 Buzzer Interfacing

Interface:

- GPIO output using `tone()`

Pin:

- `D0`

Software behavior:

- sound effects are defined as note-duration sequences
- playback is stepped forward without blocking the loop

---

## 10. Networking and Communication Protocols

This project does not use IP networking such as Wi-Fi, BLE, Ethernet, TCP, or MQTT. However, it does use embedded peripheral communication protocols, especially **I2C**, which is a networked shared-bus protocol at the board level.

### 10.1 I2C Protocol

I2C is the most important communication protocol in this project.

Devices on the I2C bus:

- SSD1306 OLED display
- RTC clock
- external EEPROM on the RTC module

Important addresses:

- OLED: `0x3C`
- RTC clock: `0x68`
- EEPROM: `0x50` to `0x57`

Why I2C is suitable here:

- only two signal lines are needed
- multiple devices can share the same bus
- widely supported by the SAMD21 and Arduino libraries

How it is used in the firmware:

- `Wire.beginTransmission(address)`
- `Wire.write(...)`
- `Wire.endTransmission()`
- `Wire.requestFrom(address, count)`
- `Wire.read()`

### 10.2 Bus-Speed Management

The firmware uses two I2C clock settings:

- a slower clock for storage and RTC access
- a faster clock for display access

This is a practical optimization because displays and RTC/EEPROM devices may have different timing sensitivities.

### 10.3 GPIO-Based Digital Signaling

The buttons, sensor, and buzzer are not network protocols in the usual sense, but they are hardware signaling interfaces:

- buttons use pull-up digital input
- sensor uses digital interrupt signaling
- buzzer uses timing-based tone output

---

## 11. Software Architecture

The software architecture is organized as a single embedded application with several logical subsystems.

### 11.1 Source Files

- `HealthPet.ino`
  - main firmware
- `GraphicsUtils.h` and `GraphicsUtils.cpp`
  - helper routines for rendering
- `Bitmap.h`
  - sprite structure definition
- `Images.h`
  - sprite and icon data in flash memory

### 11.2 Main Subsystems

1. hardware setup
2. timekeeping
3. persistence
4. pet simulation
5. activity sensing
6. button input
7. rendering
8. audio

### 11.3 Architectural Style

The system follows an event-driven, loop-based embedded architecture:

- interrupt for raw sensor capture
- foreground loop for processing
- time-triggered updates
- state-driven rendering

This is appropriate because:

- the system is small
- there is no RTOS
- hard scheduling demands are moderate
- tasks are cooperative and bounded

---

## 12. Data Structures and State Model

The project uses compact C++ structs and enums to hold state efficiently.

### 12.1 UI and state enums

- `ScreenMode`
- `InventorySelection`
- `HealthTier`
- `SaveReadStatus`

These create readable and bounded state values.

### 12.2 Button and sound structs

- `ButtonState`
- `ToneStep`
- `SoundEffect`

These support human interaction and feedback.

### 12.3 Animation structs

- `AnimationClip`
- `SpriteTierSet`

These connect asset storage to time-based animation playback.

### 12.4 Persistence struct

- `SaveData`

This is the serialized state stored in EEPROM.

### 12.5 Main runtime struct

- `PetState`

This structure is the heart of the live simulation and contains:

- health
- affection
- inventory
- activity history
- current UI mode
- temporary animation deadlines
- reward pacing state
- dirty flag
- status message

This is a strong architectural choice because all live high-level state is centralized and easy to save or restore.

---

## 13. Programming Logic

Programming logic in this project can be understood as a cycle of:

```text
Sense -> Interpret -> Update State -> Render -> Save -> Sleep
```

### 13.1 Startup Logic

`setup()` performs:

- serial startup
- pin configuration
- random seed initialization
- I2C startup
- interrupt attachment
- display initialization
- RTC/EEPROM probing
- state load

### 13.2 Loop Logic

`loop()` performs:

1. get current time
2. update game clock
3. debounce buttons
4. process sensor data
5. handle user actions
6. update sound playback
7. save if needed
8. refresh display if needed
9. sleep until the next interrupt

### 13.3 Time Progression Logic

The game clock is not based on display frames. It is based on accumulated milliseconds.

The firmware:

- accumulates elapsed real time
- converts it to virtual minutes
- applies hourly and daily state changes

This makes the simulation independent of frame rate.

### 13.4 State Progression Logic

The project encodes these ideas:

- activity improves long-term outcomes
- inactivity causes penalties
- rewards are gated and spaced
- daily performance influences future expectations

### 13.5 UI Logic

The UI is screen-based:

- home
- stats
- inventory

Each screen has a dedicated draw routine and shares a common top status bar.

---

## 14. Detailed Functional Blocks Used in the Project

This section lists the major software blocks and their purpose.

### 14.1 Interrupt and Concurrency Block

Key routines:

- `onSensorInterrupt()`
- `saveAndDisableInterrupts()`
- `restoreInterrupts()`
- `updateSensor()`

Purpose:

- capture raw physical activity safely
- isolate ISR work from high-level game logic

### 14.2 Activity Computation Block

Key routines:

- `onActivityPulse()`
- `rollingAverage()`
- `activityTarget()`
- `todayProgressPercent()`

Purpose:

- convert raw pulses into meaningful progress
- personalize the target

### 14.3 Persistence Block

Key routines:

- `encodeSaveData()`
- `decodeSaveDataDetailed()`
- `readSaveDataWithRetry()`
- `writeSaveDataWithRetry()`
- `saveState()`
- `loadState()`

Purpose:

- preserve state across resets
- validate data integrity

### 14.4 RTC Block

Key routines:

- `readRtcClockMinutes()`
- `writeRtcClockMinutes()`
- `rtcMinutesFromDateTime()`
- `rtcDateTimeFromMinutes()`
- `syncRtcClockToGameTime()`

Purpose:

- maintain persistent time
- support offline progress recovery

### 14.5 Simulation Block

Key routines:

- `completeDay()`
- `applyActivityPaceDrain()`
- `advanceGameMinute()`
- `advanceGameMinutes()`
- `updateClock()`

Purpose:

- evolve pet state over time
- create consequences for inactivity

### 14.6 Input Block

Key routines:

- `updateButton()`
- `handleInput()`
- `cycleScreen()`
- `petThePet()`
- `useInventoryItem()`

Purpose:

- map physical buttons to meaningful interaction

### 14.7 Reward and Inventory Block

Key routines:

- `addReward()`
- `maybeDropReward()`

Purpose:

- provide positive reinforcement
- make activity feel rewarding

### 14.8 Presentation Block

Key routines:

- `activeClip()`
- `currentFrame()`
- `drawTopBar()`
- `drawStatusLine()`
- `drawHomeScreen()`
- `drawStatsScreen()`
- `drawInventoryScreen()`
- `drawUi()`

Purpose:

- translate internal state into human-visible output

### 14.9 Audio Block

Key routines:

- `playSound()`
- `updateSound()`

Purpose:

- reinforce interaction and state changes acoustically

---

## 15. Memory Organization and Run-Time Environment

The target MCU has:

- `256 KB` flash
- `32 KB` RAM

The final default `-Os` build uses:

- `.text = 57868 bytes`
- `.data = 668 bytes`
- `.bss = 3280 bytes`

So static RAM usage at startup is:

```text
668 + 3280 = 3948 bytes
```

### 15.1 Static Allocation Strategy

The firmware uses static allocation rather than application-level dynamic allocation.

No application-level uses of:

- `new`
- `delete`
- `malloc`
- `calloc`
- `realloc`
- `free`

were found in the project firmware source.

This is appropriate because:

- RAM is limited
- fragmentation should be avoided
- static memory is easier to verify

### 15.2 Memory Placement Strategy

- code and constants -> flash
- sprite data -> flash (`PROGMEM`)
- initialized globals -> `.data`
- zero-initialized globals -> `.bss`
- local temporaries -> stack

### 15.3 Important Global RAM Objects

From the real object file:

- `display` occupies `108` bytes
- `pet` occupies `88` bytes
- button state objects occupy `8` bytes each in initialized data

These are modest sizes and confirm that the design is memory-conscious.

---

## 16. Debugging Strategy and Debugging Process

Debugging in embedded systems must be deliberate because errors may arise from:

- timing
- interrupts
- bus communication
- state corruption
- power-cycle recovery
- memory misuse

### 16.1 Debugging Methods Used or Supported by the Project

1. **Serial debug output**
   - startup prints RTC and EEPROM detection status
   - invalid save conditions can be reported

2. **Display-level observation**
   - status messages help reveal transitions
   - save markers show dirty, saved, and failure states

3. **State verification**
   - save writeback is re-read and compared

4. **Structured logic isolation**
   - interrupt logic is separated from foreground logic

5. **Static disassembly and map-file inspection**
   - used to validate optimization and memory layout

### 16.2 Typical Debugging Scenarios

#### A. Sensor not producing activity

Debug path:

- verify interrupt attachment
- verify pin wiring and pull-up
- inspect debounce timing
- inspect `pendingSensorActiveEdges`
- ensure `petUnavailable()` is not preventing updates

#### B. RTC or save not working

Debug path:

- verify I2C wiring
- verify RTC address `0x68`
- verify EEPROM response in `0x50` to `0x57`
- observe serial messages
- check checksum validation path

#### C. UI not refreshing properly

Debug path:

- verify display initialization
- verify `drawUi()` scheduling
- verify I2C speed switching

#### D. State unexpectedly resetting

Debug path:

- verify save write success
- verify save readback comparison
- inspect save version, magic, and checksum

### 16.3 Why this debugging approach is appropriate

This project uses lightweight but effective debugging techniques suitable for a small embedded target without a complex debug UI.

---

## 17. Optimization Strategy

Optimization in this project is both **source-level** and **compiler-level**.

### 17.1 Source-Level Optimization Strategies

#### A. Short ISR design

`onSensorInterrupt()` does minimal work:

- debounce timing
- pin sampling
- counter increment

This minimizes interrupt overhead.

#### B. Deferred processing

Activity interpretation happens in `updateSensor()` and `onActivityPulse()`, not in the ISR.

#### C. Dirty-flag save policy

State is saved only when necessary.

#### D. Display refresh throttling

Display updates occur at a controlled interval rather than every loop iteration.

#### E. Flash placement of assets

Sprites and many constants stay in flash.

#### F. Fixed-size data structures

This improves predictability and memory efficiency.

### 17.2 Compiler-Level Optimization Strategies

Measured builds:

- `-O0`
- `-O2`
- `-Os`

Real results:

| Build | `.text` size |
|---|---:|
| `-O0` | 87324 bytes |
| `-O2` | 62068 bytes |
| `-Os` | 57868 bytes |

### 17.3 Per-function optimization effects

| Function | `-O0` | `-O2` | `-Os` |
|---|---:|---:|---:|
| `onSensorInterrupt()` | `0xc8` | `0x70` | `0x64` |
| `updateSensor()` | `0x68` | `0x34` | `0x38` |
| `onActivityPulse()` | `0x11c` | `0xe4` | `0xd4` |
| `rollingAverage()` | `0x60` | `0x2c` | `0x2c` |

### 17.4 Optimization mechanisms observed

- better register allocation
- inlining of small helpers
- elimination of stack temporaries
- shorter branches and early exits
- size-aware code generation under `-Os`

### 17.5 Remaining costs

Some arithmetic still compiles to helper calls:

- `__udivsi3`
- `__aeabi_uidivmod`

These are visible in:

- `rollingAverage()`
- `activityTarget()`
- `todayProgressPercent()`
- `onActivityPulse()`

This is expected for nontrivial integer division on Cortex-M0+.

---

## 18. Final Working Model

The final working model is a compact interactive embedded device with the following behavior:

### 18.1 User experience

1. The device powers on.
2. The OLED initializes and shows the pet.
3. Saved state is restored.
4. RTC time is checked and the game catches up if needed.
5. The pet reflects current health and affection.
6. User movement generates activity.
7. Activity improves the pet’s state and can trigger rewards.
8. Buttons allow navigation and interaction.
9. Treats and gifts can be used from inventory.
10. The pet’s condition changes across the day depending on user engagement.

### 18.2 Physical final model blocks

- XIAO SAMD21 controller board
- OLED display
- vibration sensor
- RTC module with EEPROM
- three buttons
- buzzer
- common 3.3 V and ground
- shared I2C bus

### 18.3 Final embedded behavior

The final model is not just a static display. It is a persistent embedded companion with:

- time continuity
- emotional-state continuity
- activity-based adaptation
- reward mechanics
- long-lived save state

---

## 19. Measured Technical Results

### 19.1 Final size-oriented firmware

Actual final `-Os` linked image:

- `.text = 57868 bytes`
- `.data = 668 bytes`
- `.bss = 3280 bytes`

### 19.2 Linked memory boundaries

From the actual map:

- `__text_start__ = 0x00002000`
- `__etext = 0x0001020c`
- `__data_start__ = 0x20000000`
- `__data_end__ = 0x2000029c`
- `__bss_start__ = 0x200002a0`
- `__bss_end__ = 0x20000f70`
- `__StackTop = 0x20008000`

### 19.3 Remaining RAM after static allocation

```text
0x20008000 - 0x20000f70 = 0x7090 = 28816 bytes
```

This indicates that the firmware fits comfortably within the RAM budget for the target board.

### 19.4 Code-generation observations

The disassembly confirms:

- efficient interrupt-state handling
- compact branch-based control flow
- helper-call-based integer division
- strong code-size improvements under optimization

---

## 20. Limitations

Like any practical embedded prototype, the project has limitations.

### 20.1 Sensor fidelity

The vibration sensor is simpler than a full IMU or step-detection sensor. It detects motion-related events, but it does not provide medically precise step counting.

### 20.2 Behavioral granularity

The emotional model is intentionally simple. It captures motivation-oriented transitions rather than rich emotional complexity.

### 20.3 No wireless connectivity

The current implementation does not sync to a phone or cloud service.

### 20.4 Limited automated testing

The repository does not yet include a formal host-side test harness or peripheral mocks.

### 20.5 Display complexity

The OLED is low resolution and still driven as a single-bit framebuffer, so expression is intentionally minimalistic. However, the actual display module is dual-color, with a yellow upper band and a blue main area, which helps separate the status region from the rest of the pet display.

---

## 21. Future Scope

The project has strong potential for extension.

### 21.1 Improved activity sensing

- replace vibration-only input with an accelerometer or IMU
- support better step estimation
- distinguish walking, running, and idle motion

### 21.2 Richer emotional model

- more pet moods
- more nuanced behavior transitions
- more contextual messaging

### 21.3 Better personalization

- longer trend windows
- adaptive schedules
- user-specific reward pacing

### 21.4 Connectivity

- Bluetooth synchronization
- phone companion app
- cloud backup of pet state

### 21.5 Expanded game mechanics

- more item types
- multiple pets
- leveling and milestones
- achievements

### 21.6 Better testability

- unit tests for time conversion and save serialization
- mocked I2C tests
- deterministic replay for sensor traces

### 21.7 Packaging and productization

- battery-backed portable enclosure
- wearability improvements
- more polished industrial design

---

## 22. Conclusion

`HealthPet` demonstrates that a compact SAMD21 microcontroller can host a complete emotionally expressive fitness companion with persistent state, personalized target logic, and hardware-integrated interaction.

The project is technically meaningful because it combines:

- embedded peripheral integration
- real-time interrupt handling
- EEPROM persistence
- RTC-backed continuity
- animation-based UI
- sound feedback
- memory-conscious architecture
- measurable optimization analysis

It is also conceptually meaningful because it translates physical activity into the wellbeing of a virtual companion, creating a feedback loop that is more personal and engaging than a simple counter.

From a systems perspective, the project successfully integrates:

- sensing
- timing
- user input
- storage
- rendering
- audio
- optimization

within the flash and RAM limits of the XIAO SAMD21.

Therefore, `HealthPet` is both a technically complete embedded implementation and a strong foundation for future work in motivational embedded interaction systems.

---

## 23. References

The conceptual motivation of the project was originally described in `Review1.pdf`, which cited the following works:

- Byrne, Shannon et al. (2012). “Caring for Mobile Phone-Based Virtual Pets Can Influence Youth Eating Behaviors”. *Journal of Children and Media* 6.1, pp. 83–99.
- Martins, C. F. et al. (2023). “Pet’s Influence on Humans’ Daily Physical Activity and Mental Health: A Meta-Analysis”. *Frontiers in Public Health* 11.
- Park, Seung-Woo et al. (2025). “Effectiveness of a Digital Game-Based Physical Activity Program (AI-FIT) on Health-Related Physical Fitness in Elementary School Children”. *Healthcare* 13.11.
- Ramsay, D. B. et al. (n.d.). *Virtual Pets and Virtual Selves as Exercise Motivation Tools*.
