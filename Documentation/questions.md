# Questions and Answers Using the Actual `HealthPet` Project

## Project Context

This document answers the requested questions using the actual `HealthPet` embedded project.

The project is a virtual-pet firmware for the **Seeed Studio XIAO SAMD21** microcontroller board. It uses:

- an **ARM Cortex-M0+** MCU
- a **128x64 SSD1306 OLED** display over I2C
- an **RTC clock** at I2C address `0x68`
- an **EEPROM device** at I2C addresses `0x50` to `0x57`
- a **vibration sensor** for activity input
- **three buttons** for user interaction
- a **buzzer** for sound effects

The main firmware file is `HealthPet.ino`. The project was compiled and analyzed using the actual installed Arduino toolchain for the XIAO SAMD21 target.

The measured binaries used in this analysis are:

- unoptimized build: `-O0`
- speed-oriented optimized build: `-O2`
- default Arduino size-oriented build: `-Os`

The actual measured code sizes are:

| Build | `.text` | `.data` | `.bss` | Total |
|---|---:|---:|---:|---:|
| `-O0` | 87324 bytes | 672 bytes | 3280 bytes | 91276 bytes |
| `-O2` | 62068 bytes | 668 bytes | 3284 bytes | 66020 bytes |
| `-Os` | 57868 bytes | 668 bytes | 3280 bytes | 61816 bytes |

---

## Question 1. Generate assembly code during compilation and provide a walkthrough for the translation of various functionalities.

### Answer

The project was compiled for the actual board target, **Seeed XIAO SAMD21**, which uses an **ARM Cortex-M0+** core. Therefore, the compiler generates **Thumb assembly**, not fixed-width 32-bit RISC instructions.

The firmware was built using commands of the following form:

```bash
arduino-cli compile -b Seeeduino:samd:seeed_XIAO_m0 .
arduino-cli compile -b Seeeduino:samd:seeed_XIAO_m0 --build-path build-analysis/O0 --clean --build-property compiler.c.elf.flags='-O0 -Wl,--gc-sections -save-temps' .
arduino-cli compile -b Seeeduino:samd:seeed_XIAO_m0 --build-path build-analysis/O2 --clean --build-property compiler.c.elf.flags='-O2 -Wl,--gc-sections -save-temps' .
```

The generated machine code was inspected with:

```bash
arm-none-eabi-objdump -d -C HealthPet.ino.elf
```

### Example 1: Translation of the sensor interrupt routine

The source routine:

```cpp
void onSensorInterrupt() {
  const uint32_t nowUs = micros();
  if ((nowUs - lastSensorInterruptUs) < SENSOR_DEBOUNCE_US) {
    return;
  }

  lastSensorInterruptUs = nowUs;
  const bool sensorLevelHigh = digitalRead(PIN_SENSOR) == HIGH;
  pendingSensorLevelHigh = sensorLevelHigh;
  ...
}
```

The actual `-Os` linked disassembly begins as:

```asm
00002784 <onSensorInterrupt()>:
    2784: b510       push    {r4, lr}
    2786: f004 fffd  bl      7784 <micros>
    278a: 4a12       ldr     r2, [pc, #72]
    278e: 6813       ldr     r3, [r2, #0]
    2790: 1ac3       subs    r3, r0, r3
    2792: 428b       cmp     r3, r1
    2794: d91d       bls.n   27d2 <onSensorInterrupt()+0x4e>
    2796: 6010       str     r0, [r2, #0]
    2798: 2001       movs    r0, #1
    279a: f005 fa5b  bl      7c54 <digitalRead>
```

Interpretation:

- `bl micros` calls the Arduino `micros()` function.
- `subs` computes the elapsed microseconds since the previous interrupt.
- `cmp` and `bls` implement the debounce check and early return.
- `str` stores the new timestamp.
- `bl digitalRead` reads the sensor pin.

This confirms that the compiler translated the C++ control flow directly into a call, arithmetic subtraction, comparison, conditional branch, and state update.

### Example 2: Translation of atomic interrupt-safe snapshot logic

The source routine:

```cpp
void updateSensor(uint32_t now) {
  const uint32_t irqState = saveAndDisableInterrupts();
  const uint32_t transitionCount = pendingSensorTransitions;
  const uint32_t activeEdgeCount = pendingSensorActiveEdges;
  const bool sensorLevelHigh = pendingSensorLevelHigh;
  pendingSensorTransitions = 0;
  pendingSensorActiveEdges = 0;
  restoreInterrupts(irqState);
  ...
}
```

The actual `-O2` disassembly contains:

```asm
00002e68 <updateSensor(unsigned long)>:
    2e6a: f3ef 8610  mrs     r6, PRIMASK
    2e6e: b672       cpsid   i
    2e76: 682b       ldr     r3, [r5, #0]
    2e78: 6811       ldr     r1, [r2, #0]
    2e7a: 7824       ldrb    r4, [r4, #0]
    2e7e: 602c       str     r4, [r5, #0]
    2e80: 6014       str     r4, [r2, #0]
    2e82: f386 8810  msr     PRIMASK, r6
```

Interpretation:

- `mrs r6, PRIMASK` saves the interrupt state.
- `cpsid i` disables interrupts.
- `ldr` and `ldrb` copy the shared ISR-updated counters and level.
- `str` resets the counters.
- `msr PRIMASK, r6` restores the previous interrupt state.

This is a direct translation of the project’s inline assembly helpers for atomic access.

### Example 3: Translation of activity-processing logic

The source routine:

```cpp
const uint32_t totalPendingSteps =
    static_cast<uint32_t>(pendingStepRemainder) + pulseCount;
const uint32_t activityGain = totalPendingSteps / STEPS_PER_ACTIVITY_POINT;
pendingStepRemainder =
    static_cast<uint8_t>(totalPendingSteps % STEPS_PER_ACTIVITY_POINT);
```

The actual `-Os` disassembly includes:

```asm
00002c14 <onActivityPulse(unsigned long, unsigned long)>:
    2c1c: 2900       cmp     r1, #0
    2c1e: d02c       beq.n   2c7a
    2c20: f7ff fda4  bl      276c <petUnavailable()>
    2c34: 18f6       adds    r6, r6, r3
    2c3a: f005 f983  bl      7f44 <__udivsi3>
    2c44: f005 fa04  bl      8050 <__aeabi_uidivmod>
```

Interpretation:

- `cmp r1, #0` checks whether `pulseCount` is zero.
- `bl petUnavailable` checks whether the pet can currently receive activity.
- `adds` computes `pendingStepRemainder + pulseCount`.
- `__udivsi3` performs unsigned integer division by `10`.
- `__aeabi_uidivmod` computes the remainder by `10`.

This shows that the compiler translated the activity-point arithmetic into standard ARM integer helper calls.

---

## Question 2. Interpret the assembly instructions for logic implementation.

### Answer

The assembly in this firmware uses standard ARM Thumb instructions to implement the project logic.

The most important instruction types observed in the generated code are:

- `bl`
  - function call
  - used for `micros`, `digitalRead`, `petUnavailable`, `__udivsi3`, and other library or helper calls

- `ldr`, `str`
  - 32-bit load/store
  - used for timestamps, counters, and global state

- `ldrb`, `strb`
  - 8-bit load/store
  - used for flags and compact state fields such as `health`, `affection`, and booleans

- `ldrh`, `strh`
  - 16-bit load/store
  - used for fields such as `highExercisePulses`

- `cmp`
  - compare two values before a branch

- `beq`, `bne`, `bhi`, `bls`
  - conditional branches that implement `if`, `else`, and loop control

- `adds`, `subs`
  - arithmetic operations for counters, elapsed time, and loop increments

- `muls`
  - multiplication, for example in percent calculations

- `mrs`
  - move special register to a general-purpose register

- `cpsid i`
  - disable interrupts

- `msr`
  - restore a special register such as `PRIMASK`

- `wfi`
  - wait for interrupt, used to idle the CPU efficiently at the end of the main loop

### Examples from the actual firmware

- In the sensor interrupt path, `subs` followed by `cmp` and `bls` implements debounce timing.
- In `todayProgressPercent()`, `muls` multiplies activity by `100` before division.
- In `updateSensor()`, `mrs`, `cpsid i`, and `msr` implement the interrupt-safe critical section.
- In `onActivityPulse()`, `__udivsi3` and `__aeabi_uidivmod` implement division and modulo operations.

---

## Question 3. Identify the optimization strategies applied by the compiler.

### Answer

The compiler applied several standard optimization strategies to the actual `HealthPet` firmware.

### 1. Register allocation

At `-O0`, the compiler stores more temporary values on the stack.

At `-O2` and `-Os`, the compiler keeps more values in registers, which reduces memory traffic and instruction count.

Observed example:

- `updateSensor()` at `-O0` uses a stack frame and explicit stores for temporaries.
- `updateSensor()` at `-O2` keeps the interrupt state and copied counters in registers.

### 2. Inlining

Small helper routines are inlined in optimized builds.

Observed example:

- `saveAndDisableInterrupts()` and `restoreInterrupts()` become direct `mrs`, `cpsid`, and `msr` instructions in optimized code.

### 3. Branch simplification and early-return optimization

Optimized builds use shorter control-flow paths.

Observed example:

- `onSensorInterrupt()` under `-O2` and `-Os` performs the debounce test and returns immediately when the condition fails.

### 4. Elimination of temporary variables

At `-O0`, temporary values are preserved in a more debug-friendly way.

At `-O2` and `-Os`, many such temporaries disappear.

### 5. Code-size optimization under `-Os`

The Arduino toolchain’s default build uses `-Os`, which produced the smallest final binary for this project.

### 6. Limited arithmetic simplification

Some arithmetic is simplified, but integer division and modulo by `10`, `12`, and `100` still compile into runtime helper calls because these operations are relatively expensive on Cortex-M0+.

---

## Question 4. Disassemble the machine code before and after optimization flags for time and space.

### Answer

The project was compiled and measured under `-O0`, `-O2`, and the default `-Os` configuration.

### Actual measured sizes

| Build | `.text` | `.data` | `.bss` | Total |
|---|---:|---:|---:|---:|
| `-O0` | 87324 bytes | 672 bytes | 3280 bytes | 91276 bytes |
| `-O2` | 62068 bytes | 668 bytes | 3284 bytes | 66020 bytes |
| `-Os` | 57868 bytes | 668 bytes | 3280 bytes | 61816 bytes |

### Per-function size comparison from the real object files

| Function | `-O0` | `-O2` | `-Os` |
|---|---:|---:|---:|
| `onSensorInterrupt()` | `0xc8` | `0x70` | `0x64` |
| `updateSensor(unsigned long)` | `0x68` | `0x34` | `0x38` |
| `onActivityPulse(unsigned long, unsigned long)` | `0x11c` | `0xe4` | `0xd4` |
| `rollingAverage()` | `0x60` | `0x2c` | `0x2c` |
| `activityTarget()` | `0x44` | `0x4c` | `0x22` |
| `todayProgressPercent()` | `0x44` | `0x68` | `0x28` |

### Interpretation

- `-O0` produces the largest code because it preserves extra stack usage, temporaries, and debug-friendly control flow.
- `-O2` reduces most hot-path routines substantially and is the best candidate for speed.
- `-Os` produces the smallest total binary and is therefore the best candidate for flash efficiency.
- Not every function becomes smaller under `-O2`.
  - `todayProgressPercent()` grows at `-O2` because the compiler chooses a faster, more inlined form.
  - `-Os` shrinks it again because size takes priority.

---

## Question 5. Analyze the combination of instructions used by the compiler for time and space efficiency.

### Answer

The generated code uses different instruction combinations depending on whether the compiler is optimizing for speed or for code size.

### Time efficiency

The speed-oriented builds improve time efficiency by:

- reducing stack traffic
- reducing helper calls where possible
- using early returns
- keeping values in registers
- inlining short critical routines

Example:

In `updateSensor()`:

- `-O0` uses helper calls for saving and restoring interrupt state.
- `-O2` emits `mrs`, `cpsid i`, and `msr` directly.

This reduces call overhead and shortens the critical section.

### Space efficiency

The size-oriented build improves code-size efficiency by:

- reducing repeated instruction sequences
- avoiding unnecessary inlining when it increases code size
- simplifying control flow

Example:

- `onSensorInterrupt()` shrinks from `0xc8` bytes at `-O0` to `0x64` bytes at `-Os`.

### Remaining costly operations

Even in optimized builds, the project still pays a cost for integer division and modulo operations.

The following helper routines remain visible in disassembly:

- `__udivsi3`
- `__aeabi_uidivmod`

These occur in:

- `onActivityPulse()`
- `rollingAverage()`
- `activityTarget()`
- `todayProgressPercent()`

Therefore, the remaining expensive operations are mostly arithmetic divisions, not control flow.

---

## Question 6. Program size = Number of instructions × Instruction size (assuming fixed 32-bit RISC instruction size).

### Answer

For this actual project, that formula is **not strictly correct**, because the target processor is **ARM Cortex-M0+ using Thumb instructions**, and Thumb uses a mixture of **16-bit and 32-bit instructions**.

Therefore, the correct practical way to measure program size for this firmware is:

```text
Program size = linked section sizes reported by the toolchain
```

For the actual project, the measured code sizes are:

- `-O0`: `.text = 87324 bytes`
- `-O2`: `.text = 62068 bytes`
- `-Os`: `.text = 57868 bytes`

If a textbook expression is still required, the more appropriate general form is:

```text
Program size = Number of instructions × Average instruction size
```

However, for this firmware, the real answer must state that instruction size is not fixed.

---

## Question 7. Program execution time = Total clock cycles × Clock period.

### Answer

This formula is conceptually correct for the `HealthPet` firmware.

The board configuration uses:

- `F_CPU = 48000000 Hz`

Therefore:

```text
Clock period = 1 / 48,000,000 seconds
             ≈ 20.83 ns
```

So:

```text
Execution time = Total clock cycles × 20.83 ns
```

The exact cycle count was not measured on hardware in this analysis. However, the disassembly and size comparison show why optimized builds are expected to run faster:

- fewer instructions in hot routines
- fewer memory accesses
- less stack traffic
- fewer helper calls
- inlined interrupt-state handling

Therefore, for this actual code:

- `-O0` is expected to be the slowest
- `-O2` is expected to be the fastest
- `-Os` is expected to be close to `-O2`, but optimized primarily for code size

---

## Question 8. Identify and apply the right efficiency strategy for various functionalities in the project.

### Answer

The project uses different efficiency strategies for different classes of functionality. These strategies are appropriate for a small embedded system with limited RAM, limited flash, and real-time sensor interaction.

### A. Sensor interrupt handling

Relevant routines:

- `onSensorInterrupt()`
- `updateSensor()`
- `onActivityPulse()`

Applied strategy:

- keep the interrupt service routine short
- perform only edge capture and debouncing inside the ISR
- defer heavy processing to foreground code
- use an interrupt-safe atomic snapshot

Reason:

- short ISRs reduce latency
- this avoids doing costly game logic during interrupt context
- it improves predictability and responsiveness

### B. Main loop execution

Relevant routines:

- `loop()`
- `updateClock()`
- `drawUi()`
- `updateSound()`

Applied strategy:

- non-blocking task scheduling
- redraw the display only at a controlled interval
- sleep while idle using `wfi`

Reason:

- saves CPU time
- reduces power consumption
- keeps the firmware responsive without an RTOS

### C. Save and EEPROM operations

Relevant routines:

- `saveState()`
- `rtcReadBlock()`
- `rtcWriteBlock()`
- `readSaveDataWithRetry()`
- `writeSaveDataWithRetry()`

Applied strategy:

- save only when data is dirty
- rate-limit save operations
- use chunked EEPROM access
- verify saved data after writing
- retry failed transport operations

Reason:

- reduces EEPROM wear
- reduces unnecessary I2C traffic
- increases reliability

### D. Display and animation

Relevant routines:

- `activeClip()`
- `currentFrame()`
- `drawHomeScreen()`
- `drawStatsScreen()`
- `drawInventoryScreen()`

Applied strategy:

- keep assets in flash memory
- use a single display buffer
- select prebuilt bitmap frames instead of generating graphics dynamically

Reason:

- flash is more abundant than RAM
- bitmap lookup is efficient
- dynamic image generation would cost more CPU time and memory

### E. Numerical game logic

Relevant routines:

- `rollingAverage()`
- `activityTarget()`
- `todayProgressPercent()`
- `applyActivityPaceDrain()`

Applied strategy:

- use integer arithmetic
- use fixed-size arrays and counters
- avoid floating-point calculations

Reason:

- integer math is simpler and more predictable on Cortex-M0+
- fixed-size structures are safer in an embedded application

### F. User input

Relevant routines:

- `updateButton()`
- `handleInput()`

Applied strategy:

- polling with software debounce
- one-shot press detection

Reason:

- button input is low frequency
- polling is simpler than dedicating interrupts to all buttons

### Conclusion

The project applies the correct efficiency strategy for each subsystem:

- short ISR for sensor capture
- deferred processing for activity logic
- throttled UI updates
- sleep while idle
- dirty-flag-based saving
- flash-based assets
- compact integer-only simulation logic

These are appropriate and efficient design choices for this MCU class.

---

## Question 9. Identify and apply the right memory allocation and optimization for various functionalities in the project.

### Answer

The project uses a **static memory allocation strategy**, which is the correct approach for a microcontroller application of this type.

### Actual allocation strategy used

The firmware does **not** use dynamic memory allocation in its own code.

No usage was found for:

- `new`
- `delete`
- `malloc`
- `calloc`
- `realloc`
- `free`

Instead, the firmware uses:

- flash memory for code, constants, and sprite assets
- `.data` in RAM for initialized global variables
- `.bss` in RAM for zero-initialized global variables
- stack memory for local variables and function calls

### Why this is the correct strategy

The XIAO SAMD21 provides:

- `256 KB` flash
- `32 KB` RAM

For such a target:

- deterministic memory use is important
- heap fragmentation should be avoided
- static and fixed-size structures are easier to verify

### Actual linked RAM usage of the project

From the final `-Os` ELF:

- `.data = 668 bytes`
- `.bss = 3280 bytes`

So the firmware uses:

```text
668 + 3280 = 3948 bytes
```

of statically allocated RAM at startup.

From the actual map file:

- `__data_start__ = 0x20000000`
- `__data_end__   = 0x2000029c`
- `__bss_start__  = 0x200002a0`
- `__bss_end__    = 0x20000f70`
- `__StackTop     = 0x20008000`

Free RAM above static allocation:

```text
0x20008000 - 0x20000f70 = 0x7090 = 28816 bytes
```

This remaining RAM is available for:

- stack growth
- any heap use performed internally by libraries

### Memory optimization already applied in the project

#### A. Compact integer widths

The project uses:

- `uint8_t` for many state fields
- `uint16_t` for medium-sized counters and timing values
- `uint32_t` only where large counters or timestamps are necessary

This is appropriate and memory-efficient.

#### B. Fixed-size arrays

Examples:

- `activityHistory[3]`
- `statusMessage[22]`

These keep RAM use bounded and predictable.

#### C. Flash-resident assets

Bitmap assets are stored in `PROGMEM`, so they occupy flash instead of RAM.

This is an important memory optimization because the project includes many sprite frames.

#### D. Single display buffer

The firmware uses the SSD1306 library’s existing display buffer and does not allocate an extra framebuffer.

#### E. Fixed save buffers

Serialization uses small local buffers with known size rather than dynamic containers.

### Examples of major RAM-resident project objects

From the generated object file:

- `display` contributes `108` bytes in `.bss`
- `pet` contributes `88` bytes in `.bss`
- each button state object contributes `8` bytes in `.data`

This shows that the project keeps its own live data structures compact.

### Conclusion

The correct memory strategy for this project is:

- static allocation
- fixed-size structures
- flash placement for assets and constants
- minimal RAM footprint
- no application-level dynamic allocation

That strategy is already applied correctly in the actual implementation.

---

## Question 10. Walk through the linker script `.ld` file for the MCU run-time environment and memory specifications.

### Answer

The actual linker script used for the project is the XIAO SAMD21 linker script supplied by the installed board package.

Its key memory declaration is:

```ld
MEMORY
{
  FLASH (rx) : ORIGIN = 0x00000000+0x2000, LENGTH = 0x00040000-0x2000
  RAM (rwx)  : ORIGIN = 0x20000000, LENGTH = 0x00008000
}
```

### Interpretation of the memory specification

#### Flash

- total device flash = `0x40000 = 262144 bytes = 256 KB`
- bootloader reservation = `0x2000 = 8192 bytes = 8 KB`
- application flash origin = `0x2000`
- application flash length = `0x3E000 = 253952 bytes`

This means the first `8 KB` of flash is reserved for the bootloader and the user program begins at address `0x00002000`.

#### RAM

- RAM origin = `0x20000000`
- RAM length = `0x8000 = 32768 bytes = 32 KB`

### Important linker sections

#### `.text`

The script places `.text` in flash:

```ld
.text : { ... } > FLASH
```

This section contains:

- the interrupt vector table
- executable code
- constructors and destructors
- read-only data

#### `.data`

The script places `.data` in RAM but initializes it from flash:

```ld
.data : AT (__etext) { ... } > RAM
```

Meaning:

- `.data` lives in RAM at run time
- its initial bytes are stored in flash right after `.text`
- startup code copies these bytes from flash to RAM

#### `.bss`

The script places `.bss` in RAM:

```ld
.bss : { ... } > RAM
```

Meaning:

- this holds zero-initialized global and static variables
- startup code clears this region before the application begins

#### `.heap`

The script defines:

```ld
.heap (COPY): { __end__ = .; __HeapLimit = .; } > RAM
```

In this project, no explicit heap section content is provided, so `__HeapLimit` becomes the address immediately after `.bss`.

#### Stack

The script defines:

```ld
__StackTop = ORIGIN(RAM) + LENGTH(RAM);
__StackLimit = __StackTop - SIZEOF(.stack_dummy);
```

Meaning:

- the stack starts at the top of RAM
- it grows downward
- the linker checks that static RAM does not already exceed the available RAM region

### Actual linker symbols from the project map file

The real map file gives:

- `__text_start__ = 0x00002000`
- `__etext       = 0x0001020c`
- `__data_start__ = 0x20000000`
- `__data_end__   = 0x2000029c`
- `__bss_start__  = 0x200002a0`
- `__bss_end__    = 0x20000f70`
- `__HeapLimit    = 0x20000f70`
- `__StackTop     = 0x20008000`

### Conclusion

The linker script establishes the MCU run-time memory environment as follows:

- application code and constants begin in flash at `0x00002000`
- initialized globals are copied into RAM at `0x20000000`
- zero-initialized globals follow in `.bss`
- stack begins at the top of RAM
- the remaining RAM between `__HeapLimit` and `__StackTop` is available for stack and any library heap use

---

## Question 11. Generate the object and `.elf` file, disassemble it, and walk through the memory layout.

### Answer

### Generated files

The analysis used the actual generated files from the build system:

- linked executable: `HealthPet.ino.elf`
- flash image: `HealthPet.ino.bin`
- map file: `HealthPet.ino.map`
- sketch object file: `HealthPet.ino.cpp.o`
- helper object file: `GraphicsUtils.cpp.o`

### Commands used

```bash
arduino-cli compile -b Seeeduino:samd:seeed_XIAO_m0 .
arm-none-eabi-size -A HealthPet.ino.elf
arm-none-eabi-size -A HealthPet.ino.cpp.o GraphicsUtils.cpp.o
arm-none-eabi-readelf -S HealthPet.ino.elf
arm-none-eabi-objdump -d -C HealthPet.ino.elf
```

### Object-file walkthrough

The object files show function-level section sizes before final linking.

Examples from the actual `HealthPet.ino.cpp.o`:

- `loadState` = `384` bytes
- `readRtcClockMinutes` = `272` bytes
- `saveState` = `232` bytes
- `writeRtcClockMinutes` = `224` bytes
- `applyActivityPaceDrain` = `216` bytes
- `onActivityPulse` = `212` bytes

This identifies the larger routines before link-time placement.

### Final ELF section layout

From the actual linked ELF:

| Section | Size | Address |
|---|---:|---:|
| `.text` | `57868` bytes | `0x00002000` |
| `.data` | `668` bytes | `0x20000000` |
| `.bss`  | `3280` bytes | `0x200002a0` |

### Flash memory layout of the final program

Application flash begins at:

```text
0x00002000
```

The final `.text` size is:

```text
0xE20C = 57868 bytes
```

Therefore the linked flash image spans approximately:

```text
0x00002000 .. 0x0001020B
```

### RAM layout of the final program

Initialized data:

```text
.data at 0x20000000, size 0x29C
```

Zero-initialized data:

```text
.bss at 0x200002A0, size 0xCD0
```

Static RAM therefore ends at:

```text
0x20000F70
```

Top of RAM:

```text
0x20008000
```

Available RAM above static allocation:

```text
0x20000F70 .. 0x20008000
```

Size of this remaining region:

```text
0x7090 = 28816 bytes
```

### Practical interpretation of the memory layout

- executable code is in flash
- constants, string literals, and sprite tables are also stored in flash
- initialized global variables are stored in `.data`
- zero-initialized global variables are stored in `.bss`
- local variables use stack space during function execution

### Disassembly walkthrough example

The linked default `-Os` build contains:

```asm
00002784 <onSensorInterrupt()>:
    2784: b510       push    {r4, lr}
    2786: f004 fffd  bl      7784 <micros>
    2796: 6010       str     r0, [r2, #0]
    279a: f005 fa5b  bl      7c54 <digitalRead>
```

This shows that:

- the function is placed at flash address `0x2784`
- external calls have been fully resolved in the ELF
- the final executable includes both project code and linked Arduino/library code

### Conclusion

The actual object files, ELF file, and map file confirm the final memory layout of the `HealthPet` firmware:

- code and constants are placed in flash starting at `0x00002000`
- initialized data is placed in RAM at `0x20000000`
- zero-initialized globals follow in `.bss`
- the project uses approximately `3948 bytes` of static RAM at startup
- approximately `28816 bytes` remain available for stack growth and any library heap usage

---

## Final Summary

Using the actual `HealthPet` project and its real build outputs, the following conclusions can be made:

1. The project targets an **ARM Cortex-M0+** MCU and generates **Thumb assembly**.
2. The compiler translates the project logic into standard calls, loads/stores, arithmetic instructions, and conditional branches.
3. Optimization significantly reduces code size and is especially effective in interrupt-safe and arithmetic-heavy routines.
4. The default Arduino `-Os` build produces the smallest binary and is appropriate for flash-constrained embedded deployment.
5. The project already applies sound embedded-system efficiency strategies:
   - short ISR design
   - deferred processing
   - non-blocking scheduling
   - idle sleep
   - flash-based assets
   - integer arithmetic
6. The memory strategy is correct for the target:
   - static allocation
   - fixed-size buffers
   - no dynamic allocation in application code
7. The linker script places:
   - application flash at `0x00002000`
   - RAM at `0x20000000`
   - stack at the top of RAM
8. The final size-optimized firmware uses:
   - `57868 bytes` of `.text`
   - `668 bytes` of `.data`
   - `3280 bytes` of `.bss`

This analysis is therefore based on the real firmware, real compiler output, real linker script, and real section layout of the project.
