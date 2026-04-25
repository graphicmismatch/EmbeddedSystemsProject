#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Bitmap.h"
#include "GraphicsUtils.h"
#include "Images.h"

constexpr uint8_t SCREEN_WIDTH = 128;
constexpr uint8_t SCREEN_HEIGHT = 64;
constexpr uint8_t OLED_ADDRESS = 0x3C;
constexpr uint8_t RTC_CLOCK_ADDRESS = 0x68;
constexpr uint8_t EEPROM_ADDRESS_MIN = 0x50;
constexpr uint8_t EEPROM_ADDRESS_MAX = 0x57;
constexpr uint8_t EEPROM_IO_CHUNK_SIZE = 16;
constexpr uint8_t EEPROM_WRITE_CYCLE_MS = 10;
constexpr uint8_t RTC_STORAGE_RETRIES = 3;
constexpr uint32_t STORAGE_I2C_CLOCK_HZ = 100000UL;
constexpr uint32_t DISPLAY_I2C_CLOCK_HZ = 400000UL;

constexpr uint8_t PIN_BUZZER = 0;
constexpr uint8_t PIN_SENSOR = 1;
constexpr uint8_t PIN_BUTTON_PREV = 2;
constexpr uint8_t PIN_BUTTON_ACTION = 3;
constexpr uint8_t PIN_BUTTON_NEXT = 6;

constexpr uint8_t MAX_STAT = 100;
constexpr uint8_t START_HEALTH = 72;
constexpr uint8_t START_AFFECTION = 60;
constexpr uint8_t INVENTORY_MAX_TOTAL = 6;

constexpr uint8_t SLEEP_START_HOUR = 22;
constexpr uint8_t SLEEP_END_HOUR = 7;

constexpr uint16_t SENSOR_DEBOUNCE_MS = 120;
constexpr uint32_t SENSOR_DEBOUNCE_US = static_cast<uint32_t>(SENSOR_DEBOUNCE_MS) * 1000UL;
constexpr uint16_t BUTTON_DEBOUNCE_MS = 35;
constexpr uint16_t ANIMATION_FRAME_MS = 360;
constexpr uint16_t HAPPY_ANIMATION_MS = 5000;
constexpr uint16_t SURPRISE_ANIMATION_MS = 3000;
constexpr uint16_t STATUS_MESSAGE_MS = 3200;
constexpr uint16_t SAVE_INDICATOR_MS = 1500;
constexpr uint16_t DISPLAY_REFRESH_MS = 41;
constexpr uint16_t MINUTES_PER_DAY = 24U * 60U;
constexpr uint16_t RTC_BASE_YEAR = 2000;
constexpr uint32_t INACTIVITY_SLEEP_MS = 2UL * 60UL * 1000UL;
constexpr uint32_t PET_COOLDOWN_MS = 15UL * 60UL * 1000UL;
constexpr uint32_t SAVE_INTERVAL_MS = 5UL * 1000UL;
constexpr uint32_t HIGH_EXERCISE_WINDOW_MS = 20UL * 1000UL;
constexpr uint32_t SOFTWARE_MINUTE_MS = 60UL * 1000UL;

constexpr uint8_t HIGH_EXERCISE_PULSES = 14;
constexpr uint8_t STEPS_PER_ACTIVITY_POINT = 8;
constexpr uint8_t REWARD_DROP_CHANCE_PERCENT = 55;
constexpr uint16_t MIN_DAILY_BASELINE = 24;
constexpr uint8_t TARGET_INCREASE_PERCENT = 5;
constexpr uint8_t TARGET_INCREASE_MIN = 3;
constexpr bool SENSOR_ACTIVE_LOW = true;

constexpr uint32_t SAVE_MAGIC = 0x48504554UL;
constexpr uint8_t SAVE_VERSION = 2;
constexpr uint8_t SAVE_HISTORY_DAYS = 3;
constexpr uint8_t SAVE_DATA_SIZE = 34;
constexpr uint8_t SAVE_SLOT_SIZE = SAVE_DATA_SIZE + 1;
constexpr uint16_t SAVE_STORAGE_OFFSET = 0;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

enum ScreenMode : uint8_t {
  SCREEN_HOME,
  SCREEN_STATS,
  SCREEN_INVENTORY
};

enum InventorySelection : uint8_t {
  SELECT_TREAT,
  SELECT_GIFT
};

enum HealthTier : uint8_t {
  TIER_LOW,
  TIER_MID,
  TIER_HEALTHY
};

struct ButtonState {
  uint8_t pin;
  bool down;
  bool pressed;
  uint32_t lastChangeMs;
};

struct ToneStep {
  uint16_t frequency;
  uint16_t durationMs;
  uint16_t gapMs;
};

struct SoundEffect {
  const ToneStep *steps;
  uint8_t length;
};

struct AnimationClip {
  const Bitmap *const *frames;
  uint8_t frameCount;
  uint16_t frameMs;
};

struct SpriteTierSet {
  AnimationClip sleep;
  AnimationClip awake;
  AnimationClip happy;
  AnimationClip surprised;
};

struct SaveData {
  uint32_t magic;
  uint8_t version;
  uint8_t health;
  uint8_t affection;
  uint8_t treats;
  uint8_t gifts;
  uint8_t historyCount;
  uint32_t activityHistory[SAVE_HISTORY_DAYS];
  uint32_t todayActivity;
  uint32_t lifetimeActivity;
  uint16_t dayCount;
  uint16_t minuteOfDay;
};

enum SaveReadStatus : uint8_t {
  SAVE_READ_OK,
  SAVE_READ_TRANSPORT_FAILED,
  SAVE_READ_CHECKSUM_FAILED
};

struct PetState {
  uint8_t health;
  uint8_t affection;
  uint8_t treats;
  uint8_t gifts;
  uint8_t historyCount;
  uint32_t activityHistory[SAVE_HISTORY_DAYS];
  volatile uint32_t todayActivity;
  volatile uint32_t lifetimeActivity;
  uint16_t lastProcessedDay;
  ScreenMode screen;
  InventorySelection inventorySelection;
  uint32_t happyUntilMs;
  uint32_t surpriseUntilMs;
  uint32_t statusUntilMs;
  uint32_t lastPetMs;
  uint32_t lastActivityMs;
  uint32_t highExerciseWindowStartMs;
  uint32_t lastRewardActivity;
  uint16_t highExercisePulses;
  bool dirty;
  char statusMessage[22];
};

constexpr ToneStep BUTTON_TONES[] = {
    {1397, 35, 10},
};

constexpr ToneStep ITEM_TONES[] = {
    {880, 60, 20},
    {1175, 70, 30},
    {1568, 110, 20},
};

constexpr ToneStep HAPPY_TONES[] = {
    {988, 55, 15},
    {1319, 55, 15},
    {1760, 90, 20},
};

constexpr ToneStep WARNING_TONES[] = {
    {392, 120, 40},
    {330, 140, 20},
};

constexpr ToneStep DEATH_TONES[] = {
    {440, 180, 40},
    {349, 220, 40},
    {262, 350, 60},
};

constexpr ToneStep LEAVE_TONES[] = {
    {784, 100, 25},
    {659, 120, 25},
    {523, 160, 40},
};

constexpr SoundEffect SOUND_BUTTON = {BUTTON_TONES, 1};
constexpr SoundEffect SOUND_ITEM = {ITEM_TONES, 3};
constexpr SoundEffect SOUND_HAPPY = {HAPPY_TONES, 3};
constexpr SoundEffect SOUND_WARNING = {WARNING_TONES, 2};
constexpr SoundEffect SOUND_DEATH = {DEATH_TONES, 3};
constexpr SoundEffect SOUND_LEAVE = {LEAVE_TONES, 3};

const Bitmap *const HEALTHY_SLEEP_FRAMES[] = {&pet_healthy_sleep_0, &pet_healthy_sleep_1};
const Bitmap *const HEALTHY_AWAKE_FRAMES[] = {&pet_healthy_awake_0, &pet_healthy_awake_1};
const Bitmap *const HEALTHY_HAPPY_FRAMES[] = {&pet_healthy_happy_0, &pet_healthy_happy_1};
const Bitmap *const HEALTHY_SURPRISED_FRAMES[] = {&pet_healthy_surprised_0, &pet_healthy_surprised_1};

const Bitmap *const MID_SLEEP_FRAMES[] = {&pet_mid_sleep_0, &pet_mid_sleep_1};
const Bitmap *const MID_AWAKE_FRAMES[] = {&pet_mid_awake_0, &pet_mid_awake_1};
const Bitmap *const MID_HAPPY_FRAMES[] = {&pet_mid_happy_0, &pet_mid_happy_1};
const Bitmap *const MID_SURPRISED_FRAMES[] = {&pet_mid_surprised_0, &pet_mid_surprised_1};

const Bitmap *const LOW_SLEEP_FRAMES[] = {&pet_low_sleep_0, &pet_low_sleep_1};
const Bitmap *const LOW_AWAKE_FRAMES[] = {&pet_low_awake_0, &pet_low_awake_1};
const Bitmap *const LOW_HAPPY_FRAMES[] = {&pet_low_happy_0, &pet_low_happy_1};
const Bitmap *const LOW_SURPRISED_FRAMES[] = {&pet_low_surprised_0, &pet_low_surprised_1};

const Bitmap *const DEAD_FRAMES[] = {&pet_dead};
const Bitmap *const LEAVE_FRAMES[] = {&pet_leave};

const SpriteTierSet SPRITE_TIERS[] = {
    {
        {LOW_SLEEP_FRAMES, 2, ANIMATION_FRAME_MS + 120},
        {LOW_AWAKE_FRAMES, 2, ANIMATION_FRAME_MS},
        {LOW_HAPPY_FRAMES, 2, ANIMATION_FRAME_MS - 40},
        {LOW_SURPRISED_FRAMES, 2, ANIMATION_FRAME_MS - 80},
    },
    {
        {MID_SLEEP_FRAMES, 2, ANIMATION_FRAME_MS + 120},
        {MID_AWAKE_FRAMES, 2, ANIMATION_FRAME_MS},
        {MID_HAPPY_FRAMES, 2, ANIMATION_FRAME_MS - 40},
        {MID_SURPRISED_FRAMES, 2, ANIMATION_FRAME_MS - 80},
    },
    {
        {HEALTHY_SLEEP_FRAMES, 2, ANIMATION_FRAME_MS + 120},
        {HEALTHY_AWAKE_FRAMES, 2, ANIMATION_FRAME_MS},
        {HEALTHY_HAPPY_FRAMES, 2, ANIMATION_FRAME_MS - 40},
        {HEALTHY_SURPRISED_FRAMES, 2, ANIMATION_FRAME_MS - 80},
    },
};

const AnimationClip DEAD_CLIP = {DEAD_FRAMES, 1, 1000};
const AnimationClip LEAVE_CLIP = {LEAVE_FRAMES, 1, 1000};

PetState pet;
uint16_t currentDayCount = 0;
uint16_t currentMinuteOfDay = 12U * 60U;
ButtonState prevButton = {PIN_BUTTON_PREV, false, false, 0};
ButtonState actionButton = {PIN_BUTTON_ACTION, false, false, 0};
ButtonState nextButton = {PIN_BUTTON_NEXT, false, false, 0};

const SoundEffect *activeSound = nullptr;
uint8_t activeSoundStep = 0;
uint32_t nextSoundChangeMs = 0;

uint8_t rtcStorageAddress = 0;
bool rtcClockAvailable = false;
uint32_t lastClockTickMs = 0;
uint32_t lastSaveMs = 0;
uint32_t lastSaveIndicatorUntilMs = 0;
uint32_t lastSaveFailureUntilMs = 0;
uint32_t lastDrawMs = 0;
uint32_t softwareMinuteAccumulatorMs = 0;
uint8_t pendingStepRemainder = 0;

volatile uint32_t pendingSensorTransitions = 0;
volatile uint32_t pendingSensorActiveEdges = 0;
volatile bool pendingSensorLevelHigh = false;
volatile uint32_t lastSensorInterruptUs = 0;

void advanceGameMinutes(uint32_t elapsedMinutes, uint32_t now, bool notify = false);
void syncRtcClockToGameTime();

inline void useStorageI2cClock() {
  Wire.setClock(STORAGE_I2C_CLOCK_HZ);
}

inline void useDisplayI2cClock() {
  Wire.setClock(DISPLAY_I2C_CLOCK_HZ);
}

inline uint32_t saveAndDisableInterrupts() {
  uint32_t primask;
  __asm__ volatile(
      "mrs %0, primask\n"
      "cpsid i\n"
      : "=r"(primask)
      :
      : "memory");
  return primask;
}

inline void restoreInterrupts(uint32_t primask) {
  __asm__ volatile(
      "msr primask, %0\n"
      :
      : "r"(primask)
      : "memory");
}

inline void waitForInterrupt() {
  __asm__ volatile("wfi");
}

uint8_t bcdToDec(uint8_t value) {
  return static_cast<uint8_t>(((value >> 4) * 10U) + (value & 0x0F));
}

uint8_t decToBcd(uint8_t value) {
  return static_cast<uint8_t>(((value / 10U) << 4) | (value % 10U));
}

bool isLeapYear(uint16_t year) {
  return (year % 4U) == 0U && (((year % 100U) != 0U) || ((year % 400U) == 0U));
}

uint8_t daysInMonth(uint16_t year, uint8_t month) {
  static const uint8_t DAYS[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (month == 2 && isLeapYear(year)) {
    return 29;
  }
  return DAYS[month - 1];
}

void onSensorInterrupt() {
  const uint32_t nowUs = micros();
  if ((nowUs - lastSensorInterruptUs) < SENSOR_DEBOUNCE_US) {
    return;
  }

  lastSensorInterruptUs = nowUs;
  const bool sensorLevelHigh = digitalRead(PIN_SENSOR) == HIGH;
  pendingSensorLevelHigh = sensorLevelHigh;
  if (pendingSensorTransitions != 0xFFFFFFFFUL) {
    pendingSensorTransitions++;
  }

  const bool sensorActive = SENSOR_ACTIVE_LOW ? !sensorLevelHigh : sensorLevelHigh;
  if (sensorActive && !petUnavailable()) {
    if (pendingSensorActiveEdges != 0xFFFFFFFFUL) {
      pendingSensorActiveEdges++;
    }
  }
}

uint8_t clampStat(int value) {
  if (value < 0) {
    return 0;
  }
  if (value > MAX_STAT) {
    return MAX_STAT;
  }
  return static_cast<uint8_t>(value);
}

void writeUint16LE(uint8_t *buffer, uint8_t &offset, uint16_t value) {
  buffer[offset++] = static_cast<uint8_t>(value & 0xFF);
  buffer[offset++] = static_cast<uint8_t>((value >> 8) & 0xFF);
}

void writeUint32LE(uint8_t *buffer, uint8_t &offset, uint32_t value) {
  buffer[offset++] = static_cast<uint8_t>(value & 0xFF);
  buffer[offset++] = static_cast<uint8_t>((value >> 8) & 0xFF);
  buffer[offset++] = static_cast<uint8_t>((value >> 16) & 0xFF);
  buffer[offset++] = static_cast<uint8_t>((value >> 24) & 0xFF);
}

uint16_t readUint16LE(const uint8_t *buffer, uint8_t &offset) {
  const uint16_t value =
      static_cast<uint16_t>(buffer[offset]) |
      (static_cast<uint16_t>(buffer[offset + 1]) << 8);
  offset += 2;
  return value;
}

uint32_t readUint32LE(const uint8_t *buffer, uint8_t &offset) {
  const uint32_t value =
      static_cast<uint32_t>(buffer[offset]) |
      (static_cast<uint32_t>(buffer[offset + 1]) << 8) |
      (static_cast<uint32_t>(buffer[offset + 2]) << 16) |
      (static_cast<uint32_t>(buffer[offset + 3]) << 24);
  offset += 4;
  return value;
}

uint8_t checksumBytes(const uint8_t *buffer, uint8_t length) {
  uint8_t checksum = 0x5A;
  for (uint8_t i = 0; i < length; ++i) {
    checksum = static_cast<uint8_t>((checksum * 33U) ^ buffer[i]);
  }
  return checksum;
}

void encodeSaveData(const SaveData &save, uint8_t *buffer) {
  uint8_t offset = 0;
  writeUint32LE(buffer, offset, save.magic);
  buffer[offset++] = save.version;
  buffer[offset++] = save.health;
  buffer[offset++] = save.affection;
  buffer[offset++] = save.treats;
  buffer[offset++] = save.gifts;
  buffer[offset++] = save.historyCount;
  for (uint8_t i = 0; i < SAVE_HISTORY_DAYS; ++i) {
    writeUint32LE(buffer, offset, save.activityHistory[i]);
  }
  writeUint32LE(buffer, offset, save.todayActivity);
  writeUint32LE(buffer, offset, save.lifetimeActivity);
  writeUint16LE(buffer, offset, save.dayCount);
  writeUint16LE(buffer, offset, save.minuteOfDay);
  buffer[offset] = checksumBytes(buffer, offset);
}

SaveReadStatus decodeSaveDataDetailed(const uint8_t *buffer, SaveData &save) {
  const uint8_t expectedChecksum = checksumBytes(buffer, SAVE_DATA_SIZE);
  if (expectedChecksum != buffer[SAVE_DATA_SIZE]) {
    return SAVE_READ_CHECKSUM_FAILED;
  }

  uint8_t offset = 0;
  save.magic = readUint32LE(buffer, offset);
  save.version = buffer[offset++];
  save.health = buffer[offset++];
  save.affection = buffer[offset++];
  save.treats = buffer[offset++];
  save.gifts = buffer[offset++];
  save.historyCount = buffer[offset++];
  for (uint8_t i = 0; i < SAVE_HISTORY_DAYS; ++i) {
    save.activityHistory[i] = readUint32LE(buffer, offset);
  }
  save.todayActivity = readUint32LE(buffer, offset);
  save.lifetimeActivity = readUint32LE(buffer, offset);
  save.dayCount = readUint16LE(buffer, offset);
  save.minuteOfDay = readUint16LE(buffer, offset);
  return SAVE_READ_OK;
}

bool saveDataEquals(const SaveData &a, const SaveData &b) {
  if (a.magic != b.magic ||
      a.version != b.version ||
      a.health != b.health ||
      a.affection != b.affection ||
      a.treats != b.treats ||
      a.gifts != b.gifts ||
      a.historyCount != b.historyCount ||
      a.todayActivity != b.todayActivity ||
      a.lifetimeActivity != b.lifetimeActivity ||
      a.dayCount != b.dayCount ||
      a.minuteOfDay != b.minuteOfDay) {
    return false;
  }

  for (uint8_t i = 0; i < SAVE_HISTORY_DAYS; ++i) {
    if (a.activityHistory[i] != b.activityHistory[i]) {
      return false;
    }
  }

  return true;
}

bool detectRtcStorageAddress() {
  useStorageI2cClock();
  for (uint8_t address = EEPROM_ADDRESS_MIN; address <= EEPROM_ADDRESS_MAX; ++address) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) {
      rtcStorageAddress = address;
      useDisplayI2cClock();
      return true;
    }
  }

  rtcStorageAddress = 0;
  useDisplayI2cClock();
  return false;
}

bool detectRtcClock() {
  useStorageI2cClock();
  Wire.beginTransmission(RTC_CLOCK_ADDRESS);
  const bool detected = Wire.endTransmission() == 0;
  useDisplayI2cClock();
  return detected;
}

bool rtcMinutesFromDateTime(uint16_t year, uint8_t month, uint8_t day,
                            uint8_t hour, uint8_t minute, uint32_t &totalMinutes) {
  if (year < RTC_BASE_YEAR || month < 1 || month > 12 || day < 1 ||
      day > daysInMonth(year, month) || hour > 23 || minute > 59) {
    return false;
  }

  uint32_t totalDays = 0;
  for (uint16_t currentYear = RTC_BASE_YEAR; currentYear < year; ++currentYear) {
    totalDays += isLeapYear(currentYear) ? 366UL : 365UL;
  }

  for (uint8_t currentMonth = 1; currentMonth < month; ++currentMonth) {
    totalDays += daysInMonth(year, currentMonth);
  }

  totalDays += static_cast<uint32_t>(day - 1U);
  totalMinutes = totalDays * MINUTES_PER_DAY +
                 static_cast<uint32_t>(hour) * 60UL +
                 static_cast<uint32_t>(minute);
  return true;
}

void rtcDateTimeFromMinutes(uint32_t totalMinutes, uint16_t &year, uint8_t &month,
                            uint8_t &day, uint8_t &hour, uint8_t &minute,
                            uint8_t &dayOfWeek) {
  uint32_t totalDays = totalMinutes / MINUTES_PER_DAY;
  const uint16_t minuteOfDay = static_cast<uint16_t>(totalMinutes % MINUTES_PER_DAY);

  year = RTC_BASE_YEAR;
  while (true) {
    const uint16_t daysThisYear = isLeapYear(year) ? 366U : 365U;
    if (totalDays < daysThisYear) {
      break;
    }
    totalDays -= daysThisYear;
    ++year;
  }

  month = 1;
  while (true) {
    const uint8_t daysThisMonth = daysInMonth(year, month);
    if (totalDays < daysThisMonth) {
      break;
    }
    totalDays -= daysThisMonth;
    ++month;
  }

  day = static_cast<uint8_t>(totalDays + 1U);
  hour = static_cast<uint8_t>(minuteOfDay / 60U);
  minute = static_cast<uint8_t>(minuteOfDay % 60U);

  // 2000-01-01 was a Saturday. DS1307 day-of-week is user-defined 1..7.
  dayOfWeek = static_cast<uint8_t>(((totalMinutes / MINUTES_PER_DAY) + 5UL) % 7UL) + 1U;
}

uint32_t gameTotalMinutes() {
  return static_cast<uint32_t>(currentDayCount) * MINUTES_PER_DAY + currentMinuteOfDay;
}

bool readRtcClockMinutes(uint32_t &totalMinutes) {
  if (!rtcClockAvailable) {
    return false;
  }

  useStorageI2cClock();
  Wire.beginTransmission(RTC_CLOCK_ADDRESS);
  Wire.write(static_cast<uint8_t>(0x00));
  if (Wire.endTransmission() != 0) {
    useDisplayI2cClock();
    return false;
  }

  if (Wire.requestFrom(RTC_CLOCK_ADDRESS, static_cast<uint8_t>(7)) != 7) {
    useDisplayI2cClock();
    return false;
  }

  const uint8_t secondsRaw = Wire.read();
  const uint8_t minutesRaw = Wire.read();
  const uint8_t hoursRaw = Wire.read();
  Wire.read(); // day-of-week
  const uint8_t dayRaw = Wire.read();
  const uint8_t monthRaw = Wire.read();
  const uint8_t yearRaw = Wire.read();
  useDisplayI2cClock();

  if ((secondsRaw & 0x80U) != 0U) {
    return false;
  }

  const uint8_t minute = bcdToDec(minutesRaw & 0x7FU);
  uint8_t hour = 0;
  if ((hoursRaw & 0x40U) != 0U) {
    hour = bcdToDec(hoursRaw & 0x1FU);
    const bool isPm = (hoursRaw & 0x20U) != 0U;
    if (hour == 12U) {
      hour = isPm ? 12U : 0U;
    } else if (isPm) {
      hour = static_cast<uint8_t>(hour + 12U);
    }
  } else {
    hour = bcdToDec(hoursRaw & 0x3FU);
  }

  const uint8_t day = bcdToDec(dayRaw & 0x3FU);
  const uint8_t month = bcdToDec(monthRaw & 0x1FU);
  const uint16_t year = static_cast<uint16_t>(RTC_BASE_YEAR + bcdToDec(yearRaw));
  return rtcMinutesFromDateTime(year, month, day, hour, minute, totalMinutes);
}

bool writeRtcClockMinutes(uint32_t totalMinutes) {
  if (!rtcClockAvailable) {
    return false;
  }

  uint16_t year = RTC_BASE_YEAR;
  uint8_t month = 1;
  uint8_t day = 1;
  uint8_t hour = 0;
  uint8_t minute = 0;
  uint8_t dayOfWeek = 1;
  rtcDateTimeFromMinutes(totalMinutes, year, month, day, hour, minute, dayOfWeek);

  useStorageI2cClock();
  Wire.beginTransmission(RTC_CLOCK_ADDRESS);
  Wire.write(static_cast<uint8_t>(0x00));
  Wire.write(decToBcd(0)); // seconds
  Wire.write(decToBcd(minute));
  Wire.write(decToBcd(hour));
  Wire.write(decToBcd(dayOfWeek));
  Wire.write(decToBcd(day));
  Wire.write(decToBcd(month));
  Wire.write(decToBcd(static_cast<uint8_t>(year - RTC_BASE_YEAR)));
  const bool success = Wire.endTransmission() == 0;
  useDisplayI2cClock();
  return success;
}

bool rtcReadBlock(uint16_t startAddress, uint8_t *buffer, uint8_t length) {
  if (rtcStorageAddress == 0) {
    return false;
  }

  useStorageI2cClock();
  uint8_t offset = 0;
  while (offset < length) {
    uint8_t chunkLength = static_cast<uint8_t>(length - offset);
    if (chunkLength > EEPROM_IO_CHUNK_SIZE) {
      chunkLength = EEPROM_IO_CHUNK_SIZE;
    }

    const uint16_t address = static_cast<uint16_t>(startAddress + offset);
    Wire.beginTransmission(rtcStorageAddress);
    Wire.write(static_cast<uint8_t>((address >> 8) & 0xFF));
    Wire.write(static_cast<uint8_t>(address & 0xFF));
    if (Wire.endTransmission() != 0) {
      useDisplayI2cClock();
      return false;
    }

    if (Wire.requestFrom(rtcStorageAddress, chunkLength) != chunkLength) {
      useDisplayI2cClock();
      return false;
    }

    for (uint8_t i = 0; i < chunkLength; ++i) {
      buffer[offset + i] = Wire.read();
    }
    offset += chunkLength;
  }
  useDisplayI2cClock();
  return true;
}

bool rtcWriteBlock(uint16_t startAddress, const uint8_t *buffer, uint8_t length) {
  if (rtcStorageAddress == 0) {
    return false;
  }

  useStorageI2cClock();
  uint8_t offset = 0;
  while (offset < length) {
    uint8_t chunkLength = static_cast<uint8_t>(length - offset);
    if (chunkLength > EEPROM_IO_CHUNK_SIZE) {
      chunkLength = EEPROM_IO_CHUNK_SIZE;
    }

    const uint16_t address = static_cast<uint16_t>(startAddress + offset);
    Wire.beginTransmission(rtcStorageAddress);
    Wire.write(static_cast<uint8_t>((address >> 8) & 0xFF));
    Wire.write(static_cast<uint8_t>(address & 0xFF));
    for (uint8_t i = 0; i < chunkLength; ++i) {
      Wire.write(buffer[offset + i]);
    }
    if (Wire.endTransmission() != 0) {
      useDisplayI2cClock();
      return false;
    }

    delay(EEPROM_WRITE_CYCLE_MS);
    offset += chunkLength;
  }
  useDisplayI2cClock();
  return true;
}

SaveReadStatus readSaveDataDetailed(SaveData &save, uint8_t *buffer) {
  if (!rtcReadBlock(SAVE_STORAGE_OFFSET, buffer, SAVE_SLOT_SIZE)) {
    return SAVE_READ_TRANSPORT_FAILED;
  }
  return decodeSaveDataDetailed(buffer, save);
}

bool writeSaveData(const SaveData &save) {
  uint8_t buffer[SAVE_SLOT_SIZE] = {};
  encodeSaveData(save, buffer);
  return rtcWriteBlock(SAVE_STORAGE_OFFSET, buffer, SAVE_SLOT_SIZE);
}

const char *validateSaveDataReason(const SaveData &saved) {
  const uint16_t inventoryTotal = static_cast<uint16_t>(saved.treats) + saved.gifts;
  if (saved.magic != SAVE_MAGIC) {
    return "magic mismatch";
  }
  if (saved.version != SAVE_VERSION) {
    return "version mismatch";
  }
  if (saved.health > MAX_STAT) {
    return "health out of range";
  }
  if (saved.affection > MAX_STAT) {
    return "affection out of range";
  }
  if (saved.historyCount > SAVE_HISTORY_DAYS) {
    return "historyCount out of range";
  }
  if (inventoryTotal > INVENTORY_MAX_TOTAL) {
    return "inventory total out of range";
  }
  if (saved.minuteOfDay >= MINUTES_PER_DAY) {
    return "minuteOfDay out of range";
  }
  return nullptr;
}

bool readSaveDataWithRetry(SaveData &save, bool verbose = true) {
  uint8_t rawBuffer[SAVE_SLOT_SIZE] = {};
  for (uint8_t attempt = 0; attempt < RTC_STORAGE_RETRIES; ++attempt) {
    const SaveReadStatus status = readSaveDataDetailed(save, rawBuffer);
    if (status == SAVE_READ_OK) {
      return true;
    }

    if (verbose && Serial) {
      if (status == SAVE_READ_TRANSPORT_FAILED) {
        Serial.println("EEPROM read failed");
      } else {
        Serial.println("EEPROM data invalid");
      }
    }
    delay(2);
  }
  return false;
}

bool writeSaveDataWithRetry(const SaveData &save) {
  for (uint8_t attempt = 0; attempt < RTC_STORAGE_RETRIES; ++attempt) {
    if (writeSaveData(save)) {
      return true;
    }
    delay(2);
  }
  return false;
}

void setStatus(const char *message, uint32_t now, uint32_t durationMs = STATUS_MESSAGE_MS) {
  snprintf(pet.statusMessage, sizeof(pet.statusMessage), "%s", message);
  pet.statusUntilMs = now + durationMs;
}

void playSound(const SoundEffect &sound) {
  activeSound = &sound;
  activeSoundStep = 0;
  nextSoundChangeMs = 0;
}

void updateSound(uint32_t now) {
  if (activeSound == nullptr || now < nextSoundChangeMs) {
    return;
  }

  if (activeSoundStep >= activeSound->length) {
    noTone(PIN_BUZZER);
    activeSound = nullptr;
    return;
  }

  const ToneStep &step = activeSound->steps[activeSoundStep++];
  if (step.frequency == 0) {
    noTone(PIN_BUZZER);
  } else {
    tone(PIN_BUZZER, step.frequency, step.durationMs);
  }

  nextSoundChangeMs = now + step.durationMs + step.gapMs;
}

bool inventoryFull() {
  return (pet.treats + pet.gifts) >= INVENTORY_MAX_TOTAL;
}

bool petDead() {
  return pet.health == 0;
}

bool petLeft() {
  return !petDead() && pet.affection == 0;
}

bool petUnavailable() {
  return petDead() || petLeft();
}

HealthTier currentTier() {
  if (pet.health >= 67) {
    return TIER_HEALTHY;
  }
  if (pet.health >= 34) {
    return TIER_MID;
  }
  return TIER_LOW;
}

bool isSleepingNow() {
  const uint8_t currentHour = static_cast<uint8_t>(currentMinuteOfDay / 60U);
  return currentHour >= SLEEP_START_HOUR || currentHour < SLEEP_END_HOUR;
}

bool isIdleSleeping(uint32_t now) {
  return (now - pet.lastActivityMs) >= INACTIVITY_SLEEP_MS;
}

uint32_t rollingAverage() {
  if (pet.historyCount == 0) {
    return MIN_DAILY_BASELINE;
  }

  uint32_t sum = 0;
  for (uint8_t i = 0; i < pet.historyCount; ++i) {
    sum += pet.activityHistory[i];
  }
  return sum / pet.historyCount;
}

uint32_t activityTarget() {
  uint32_t base = rollingAverage();
  if (base < MIN_DAILY_BASELINE) {
    base = MIN_DAILY_BASELINE;
  }

  uint32_t increase = (base * TARGET_INCREASE_PERCENT) / 100;
  if (increase < TARGET_INCREASE_MIN) {
    increase = TARGET_INCREASE_MIN;
  }
  return base + increase;
}

uint8_t todayProgressPercent() {
  const uint32_t target = activityTarget();
  if (target == 0) {
    return 0;
  }

  const uint32_t percent = (pet.todayActivity * 100UL) / target;
  return static_cast<uint8_t>(percent > 100 ? 100 : percent);
}

uint16_t awakeMinutesElapsed(uint16_t minuteOfDay) {
  const uint16_t wakeStartMinute = static_cast<uint16_t>(SLEEP_END_HOUR) * 60U;
  const uint16_t sleepStartMinute = static_cast<uint16_t>(SLEEP_START_HOUR) * 60U;

  if (minuteOfDay <= wakeStartMinute) {
    return 0;
  }
  if (minuteOfDay >= sleepStartMinute) {
    return sleepStartMinute - wakeStartMinute;
  }
  return minuteOfDay - wakeStartMinute;
}

uint16_t totalAwakeMinutes() {
  return static_cast<uint16_t>(SLEEP_START_HOUR - SLEEP_END_HOUR) * 60U;
}

void markDirty() {
  pet.dirty = true;
}

void setDefaultState() {
  pet.health = START_HEALTH;
  pet.affection = START_AFFECTION;
  pet.treats = 0;
  pet.gifts = 0;
  pet.historyCount = 0;
  for (uint8_t i = 0; i < SAVE_HISTORY_DAYS; ++i) {
    pet.activityHistory[i] = 0;
  }
  pet.todayActivity = 0;
  pet.lifetimeActivity = 0;
  pet.lastProcessedDay = currentDayCount;
  pet.screen = SCREEN_HOME;
  pet.inventorySelection = SELECT_TREAT;
  pet.happyUntilMs = 0;
  pet.surpriseUntilMs = 0;
  pet.statusUntilMs = 0;
  pet.lastPetMs = 0;
  pet.lastActivityMs = 0;
  pet.highExerciseWindowStartMs = 0;
  pet.lastRewardActivity = 0;
  pet.highExercisePulses = 0;
  pet.dirty = false;
  pet.statusMessage[0] = '\0';
  pendingStepRemainder = 0;
}

bool isValidSaveData(const SaveData &saved) {
  return validateSaveDataReason(saved) == nullptr;
}

void loadState(uint32_t now) {
  setDefaultState();

  SaveData saved = {};
  bool loadedSave = false;
  bool rtcSeededFromSave = false;
  bool rtcInitializedOnBoot = false;
  if (readSaveDataWithRetry(saved)) {
    if (isValidSaveData(saved)) {
      pet.health = saved.health;
      pet.affection = saved.affection;
      pet.treats = saved.treats;
      pet.gifts = saved.gifts;
      pet.historyCount = saved.historyCount;
      for (uint8_t i = 0; i < SAVE_HISTORY_DAYS; ++i) {
        pet.activityHistory[i] = saved.activityHistory[i];
      }
      pet.todayActivity = saved.todayActivity;
      pet.lifetimeActivity = saved.lifetimeActivity;
      currentDayCount = saved.dayCount;
      currentMinuteOfDay = saved.minuteOfDay;
      pet.lastProcessedDay = saved.dayCount;
      loadedSave = true;
    } else {
      if (Serial) {
        Serial.print("EEPROM save ignored: ");
        Serial.println(validateSaveDataReason(saved));
      }
    }
  } else if (Serial) {
    Serial.println("No valid EEPROM save found");
  }

  const uint32_t savedTotalMinutes = gameTotalMinutes();
  if (rtcClockAvailable) {
    uint32_t rtcMinutes = 0;
    const bool rtcTimeValid = readRtcClockMinutes(rtcMinutes);
    if (loadedSave && rtcTimeValid) {
      if (rtcMinutes >= savedTotalMinutes) {
        advanceGameMinutes(rtcMinutes - savedTotalMinutes, now, false);
      } else if (writeRtcClockMinutes(savedTotalMinutes)) {
        rtcSeededFromSave = true;
        rtcInitializedOnBoot = true;
      } else {
        rtcClockAvailable = false;
      }
    } else if (writeRtcClockMinutes(savedTotalMinutes)) {
      rtcSeededFromSave = loadedSave;
      rtcInitializedOnBoot = true;
    } else {
      rtcClockAvailable = false;
    }
  }

  if (petDead()) {
    setStatus("Your pet died", now, 6000);
  } else if (petLeft()) {
    setStatus("Your pet left", now, 6000);
  } else if (loadedSave && rtcSeededFromSave) {
    setStatus("RTC reset, using save", now, 2000);
  } else if (rtcInitializedOnBoot) {
    setStatus("RTC initialized", now, 2000);
  } else if (!rtcClockAvailable) {
    setStatus("RTC unavailable", now, 2000);
  }
}

void restartPet(uint32_t now) {
  currentDayCount = 0;
  currentMinuteOfDay = 12U * 60U;
  softwareMinuteAccumulatorMs = 0;
  setDefaultState();
  pet.lastActivityMs = now;
  pet.highExerciseWindowStartMs = now;
  pet.screen = SCREEN_HOME;
  markDirty();
  setStatus("New pet arrived", now, 2000);
  triggerHappy(now);
  playSound(SOUND_HAPPY);
  syncRtcClockToGameTime();
}

void saveState(bool force) {
  const uint32_t now = millis();
  if (!force && (!pet.dirty || (now - lastSaveMs) < SAVE_INTERVAL_MS)) {
    return;
  }

  SaveData out = {};
  out.magic = SAVE_MAGIC;
  out.version = SAVE_VERSION;
  out.health = pet.health;
  out.affection = pet.affection;
  out.treats = pet.treats;
  out.gifts = pet.gifts;
  out.historyCount = pet.historyCount;
  for (uint8_t i = 0; i < SAVE_HISTORY_DAYS; ++i) {
    out.activityHistory[i] = pet.activityHistory[i];
  }
  out.todayActivity = pet.todayActivity;
  out.lifetimeActivity = pet.lifetimeActivity;
  out.dayCount = currentDayCount;
  out.minuteOfDay = currentMinuteOfDay;

  if (!writeSaveDataWithRetry(out)) {
    lastSaveFailureUntilMs = now + SAVE_INDICATOR_MS;
    return;
  }

  SaveData verify = {};
  if (!readSaveDataWithRetry(verify, false) || !saveDataEquals(out, verify)) {
    lastSaveFailureUntilMs = now + SAVE_INDICATOR_MS;
    return;
  }

  pet.dirty = false;
  lastSaveMs = now;
  lastSaveIndicatorUntilMs = now + SAVE_INDICATOR_MS;
  lastSaveFailureUntilMs = 0;
}

void pushCompletedDay(uint32_t dayActivity) {
  if (pet.historyCount < SAVE_HISTORY_DAYS) {
    pet.activityHistory[pet.historyCount++] = dayActivity;
    return;
  }

  for (uint8_t i = 1; i < SAVE_HISTORY_DAYS; ++i) {
    pet.activityHistory[i - 1] = pet.activityHistory[i];
  }
  pet.activityHistory[SAVE_HISTORY_DAYS - 1] = dayActivity;
}

void triggerHappy(uint32_t now) {
  pet.happyUntilMs = now + HAPPY_ANIMATION_MS;
}

void triggerSurprise(uint32_t now) {
  pet.surpriseUntilMs = now + SURPRISE_ANIMATION_MS;
}

void completeDay(uint32_t dayActivity, uint32_t now, bool notify = true) {
  const uint32_t target = activityTarget();
  const uint32_t percent = target == 0 ? 0 : (dayActivity * 100UL) / target;
  pushCompletedDay(dayActivity);
  pet.todayActivity = 0;
  pet.lastRewardActivity = 0;
  pet.highExercisePulses = 0;
  pet.highExerciseWindowStartMs = now;
  markDirty();

  if (!notify) {
    return;
  }

  if (percent >= 100) {
    setStatus("Great activity day", now);
    triggerHappy(now);
    playSound(SOUND_HAPPY);
  } else if (percent < 70) {
    setStatus("Move a bit more", now);
    playSound(SOUND_WARNING);
  }
}

void applyActivityPaceDrain(uint32_t now, bool notify = true) {
  if (petUnavailable()) {
    return;
  }
  if (isSleepingNow()) {
    return;
  }

  const uint32_t target = activityTarget();
  const uint16_t awakeElapsed = awakeMinutesElapsed(currentMinuteOfDay);
  const uint16_t awakeTotal = totalAwakeMinutes();
  if (target == 0 || awakeElapsed == 0 || awakeTotal == 0) {
    return;
  }

  const uint32_t expectedActivity =
      ((target * awakeElapsed) + awakeTotal - 1U) / awakeTotal;
  if (expectedActivity == 0) {
    return;
  }

  const uint32_t pacePercent = (pet.todayActivity * 100UL) / expectedActivity;
  int healthDelta = 0;
  int affectionDelta = 0;

  if (pacePercent >= 100UL) {
    affectionDelta = -1;
    healthDelta = -1;
  } else if (pacePercent >= 75UL) {
    affectionDelta = -1;
    healthDelta = -2;
  } else if (pacePercent >= 50UL) {
    healthDelta = -3;
    affectionDelta = -1;
  } else {
    healthDelta = -4;
    affectionDelta = -3;
  }

  pet.health = clampStat(static_cast<int>(pet.health) + healthDelta);
  pet.affection = clampStat(static_cast<int>(pet.affection) + affectionDelta);
  markDirty();

  if (!notify) {
    return;
  }

  if (petDead()) {
    setStatus("Your pet died", now, 6000);
    playSound(SOUND_DEATH);
    return;
  }

  if (petLeft()) {
    setStatus("Your pet left", now, 6000);
    playSound(SOUND_LEAVE);
  }
}

void syncRtcClockToGameTime() {
  if (!rtcClockAvailable) {
    return;
  }

  if (!writeRtcClockMinutes(gameTotalMinutes())) {
    rtcClockAvailable = false;
  }
}

void advanceGameMinute(uint32_t now, bool notify = true) {
  currentMinuteOfDay++;
  if (currentMinuteOfDay >= MINUTES_PER_DAY) {
    completeDay(pet.todayActivity, now, notify);
    currentMinuteOfDay = 0;
    currentDayCount++;
    markDirty();
  }

  if ((currentMinuteOfDay % 30U) == 0) {
    applyActivityPaceDrain(now, notify);
  }
}

void advanceGameMinutes(uint32_t elapsedMinutes, uint32_t now, bool notify) {
  while (elapsedMinutes > 0) {
    advanceGameMinute(now, notify);
    --elapsedMinutes;
  }
}

void updateClock(uint32_t now) {
  const uint32_t elapsed = now - lastClockTickMs;
  lastClockTickMs = now;

  softwareMinuteAccumulatorMs += elapsed;
  while (softwareMinuteAccumulatorMs >= SOFTWARE_MINUTE_MS) {
    softwareMinuteAccumulatorMs -= SOFTWARE_MINUTE_MS;
    advanceGameMinute(now);
    syncRtcClockToGameTime();
  }
}

void updateButton(ButtonState &button, uint32_t now) {
  button.pressed = false;
  const bool reading = digitalRead(button.pin) == LOW;

  if (reading != button.down && (now - button.lastChangeMs) >= BUTTON_DEBOUNCE_MS) {
    button.down = reading;
    button.lastChangeMs = now;
    if (reading) {
      button.pressed = true;
    }
  }
}

void addReward(bool gift, uint32_t now) {
  if (inventoryFull()) {
    return;
  }

  if (gift) {
    pet.gifts++;
    setStatus("Found a gift", now);
  } else {
    pet.treats++;
    setStatus("Found a treat", now);
  }

  markDirty();
  triggerSurprise(now);
  playSound(SOUND_ITEM);
}

void maybeDropReward(uint32_t now) {
  if (inventoryFull()) {
    return;
  }

  const uint32_t target = activityTarget();
  if (pet.todayActivity < (target / 2)) {
    return;
  }

  const uint32_t rewardSpacing = target / 3 > 18 ? target / 3 : 18;
  if (pet.todayActivity < (pet.lastRewardActivity + rewardSpacing)) {
    return;
  }

  pet.lastRewardActivity = pet.todayActivity;
  if (random(100) < REWARD_DROP_CHANCE_PERCENT) {
    addReward(random(100) < 30, now);
  } else {
    triggerSurprise(now);
  }
}

void onActivityPulse(uint32_t now, uint32_t pulseCount) {
  if (pulseCount == 0 || petUnavailable()) {
    return;
  }

  pet.lastActivityMs = now;
  const uint32_t totalPendingSteps =
      static_cast<uint32_t>(pendingStepRemainder) + pulseCount;
  const uint32_t activityGain = totalPendingSteps / STEPS_PER_ACTIVITY_POINT;
  pendingStepRemainder =
      static_cast<uint8_t>(totalPendingSteps % STEPS_PER_ACTIVITY_POINT);

  if ((now - pet.highExerciseWindowStartMs) > HIGH_EXERCISE_WINDOW_MS) {
    pet.highExerciseWindowStartMs = now;
    pet.highExercisePulses = 0;
  }

  const uint32_t startingTodayActivity = pet.todayActivity;
  if (activityGain > 0) {
    pet.todayActivity += activityGain;
    pet.lifetimeActivity += activityGain;
    markDirty();
  }

  for (uint32_t point = 0; point < activityGain; ++point) {
    const uint32_t activityTotal = startingTodayActivity + point + 1;
    if ((activityTotal % 12UL) == 0) {
      triggerHappy(now);
    }
  }

  for (uint32_t pulse = 0; pulse < pulseCount; pulse++) {
    pet.highExercisePulses++;
    if (pet.highExercisePulses >= HIGH_EXERCISE_PULSES) {
      pet.highExercisePulses = 0;
      pet.highExerciseWindowStartMs = now;
      maybeDropReward(now);
    }
  }
}

void updateSensor(uint32_t now) {
  const uint32_t irqState = saveAndDisableInterrupts();
  const uint32_t transitionCount = pendingSensorTransitions;
  const uint32_t activeEdgeCount = pendingSensorActiveEdges;
  const bool sensorLevelHigh = pendingSensorLevelHigh;
  pendingSensorTransitions = 0;
  pendingSensorActiveEdges = 0;
  restoreInterrupts(irqState);

  if (transitionCount == 0 && activeEdgeCount == 0) {
    return;
  }

  onActivityPulse(now, activeEdgeCount);
}

void cycleScreen(int8_t direction, uint32_t now) {
  int8_t next = static_cast<int8_t>(pet.screen) + direction;
  if (next < 0) {
    next = 2;
  } else if (next > 2) {
    next = 0;
  }
  pet.screen = static_cast<ScreenMode>(next);
  playSound(SOUND_BUTTON);
  setStatus(
      pet.screen == SCREEN_HOME ? "Home" :
      pet.screen == SCREEN_STATS ? "Stats" : "Inventory",
      now,
      1000);
}

void petThePet(uint32_t now) {
  if (petUnavailable()) {
    restartPet(now);
    return;
  }

  if ((now - pet.lastPetMs) < PET_COOLDOWN_MS) {
    setStatus("Try later", now, 1200);
    playSound(SOUND_BUTTON);
    return;
  }

  pet.lastPetMs = now;
  pet.affection = clampStat(static_cast<int>(pet.affection) + 2);
  markDirty();
  triggerHappy(now);
  setStatus("Pet likes that", now);
  playSound(SOUND_HAPPY);
}

void useInventoryItem(uint32_t now) {
  if (petUnavailable()) {
    return;
  }

  if (pet.inventorySelection == SELECT_TREAT) {
    if (pet.treats == 0) {
      setStatus("No treats left", now, 1200);
      playSound(SOUND_BUTTON);
      return;
    }

    pet.treats--;
    pet.affection = clampStat(static_cast<int>(pet.affection) + 8);
    pet.health = clampStat(static_cast<int>(pet.health) + 3);
    setStatus("Treat given", now);
  } else {
    if (pet.gifts == 0) {
      setStatus("No gifts left", now, 1200);
      playSound(SOUND_BUTTON);
      return;
    }

    pet.gifts--;
    pet.affection = clampStat(static_cast<int>(pet.affection) + 16);
    pet.health = clampStat(static_cast<int>(pet.health) + 1);
    setStatus("Gift delivered", now);
  }

  markDirty();
  triggerHappy(now);
  playSound(SOUND_ITEM);
}

void handleInput(uint32_t now) {
  if (prevButton.pressed) {
    if (pet.screen == SCREEN_INVENTORY && pet.inventorySelection == SELECT_GIFT) {
      pet.inventorySelection = SELECT_TREAT;
      playSound(SOUND_BUTTON);
    } else {
      cycleScreen(-1, now);
    }
  }

  if (nextButton.pressed) {
    if (pet.screen == SCREEN_INVENTORY && pet.inventorySelection == SELECT_TREAT) {
      pet.inventorySelection = SELECT_GIFT;
      playSound(SOUND_BUTTON);
    } else {
      cycleScreen(1, now);
    }
  }

  if (actionButton.pressed) {
    if (pet.screen == SCREEN_HOME) {
      petThePet(now);
    } else if (pet.screen == SCREEN_INVENTORY) {
      useInventoryItem(now);
    } else {
      setStatus("3d avg vs target", now, 1300);
      playSound(SOUND_BUTTON);
    }
  }
}

const AnimationClip &activeClip(uint32_t now) {
  if (petDead()) {
    return DEAD_CLIP;
  }

  if (petLeft()) {
    return LEAVE_CLIP;
  }

  const SpriteTierSet &tier = SPRITE_TIERS[currentTier()];
  if (now < pet.surpriseUntilMs) {
    return tier.surprised;
  }
  if (now < pet.happyUntilMs) {
    return tier.happy;
  }
  if (isIdleSleeping(now)) {
    return tier.sleep;
  }
  return tier.awake;
}

const Bitmap *currentFrame(uint32_t now) {
  const AnimationClip &clip = activeClip(now);
  const uint8_t frameIndex = clip.frameCount <= 1 ? 0 : (now / clip.frameMs) % clip.frameCount;
  return clip.frames[frameIndex];
}

void printAge() {
  if (currentDayCount >= 7U) {
    display.print(currentDayCount / 7U);
    display.print("w");
  } else {
    display.print(currentDayCount);
    display.print("d");
  }
}

void drawTopBar() {
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("H");
  drawProgressBar(display, 8, 1, 40, 7, pet.health);

  display.setCursor(52, 0);
  display.print("A");
  drawProgressBar(display, 60, 1, 30, 7, pet.affection);

  display.setCursor(94, 0);
  display.print(todayProgressPercent());
  display.print("%");

  display.drawFastHLine(0, 10, SCREEN_WIDTH, SSD1306_WHITE);
}

void drawSaveIndicator(uint32_t now) {
  display.setCursor(122, 16);
  if (lastSaveFailureUntilMs > now) {
    display.print("!");
  } else if (pet.dirty) {
    display.print("*");
  } else if (lastSaveIndicatorUntilMs > now) {
    display.print("S");
  }
}

void drawStatusLine(uint32_t now) {
  display.setCursor(0, 54);
  if (now < pet.statusUntilMs && pet.statusMessage[0] != '\0') {
    display.print(pet.statusMessage);
    return;
  }

  display.print("Act ");
  display.print(pet.todayActivity);
  display.print("/");
  display.print(activityTarget());
  display.setCursor(88, 54);
  printAge();
}

void drawHomeScreen(uint32_t now) {
  drawTopBar();
  drawBitmapCenteredAt(display, 64, 33, currentFrame(now));
  drawStatusLine(now);
}

void drawStatsScreen() {
  drawTopBar();
  display.setCursor(0, 16);
  display.print("Today: ");
  display.print(pet.todayActivity);

  display.setCursor(0, 27);
  display.print("Target: ");
  display.print(activityTarget());

  display.setCursor(0, 38);
  display.print("3d avg: ");
  display.print(rollingAverage());

  display.setCursor(0, 49);
  display.print("Age: ");
  printAge();
}

void drawInventoryScreen() {
  drawTopBar();
  display.setCursor(0, 16);
  display.print("Items");

  display.setCursor(0, 28);
  display.print(pet.inventorySelection == SELECT_TREAT ? ">" : " ");
  display.print(" Treats x");
  display.print(pet.treats);

  display.setCursor(0, 40);
  display.print(pet.inventorySelection == SELECT_GIFT ? ">" : " ");
  display.print(" Gifts  x");
  display.print(pet.gifts);

  display.setCursor(0, 54);
  display.print("Press action to use");
}

void drawUi(uint32_t now) {
  display.clearDisplay();

  if (pet.screen == SCREEN_HOME) {
    drawHomeScreen(now);
  } else if (pet.screen == SCREEN_STATS) {
    drawStatsScreen();
  } else {
    drawInventoryScreen();
  }

  drawSaveIndicator(now);
  useDisplayI2cClock();
  display.display();
}

void setup() {
  Serial.begin(115200);
  const uint32_t serialWaitStart = millis();
  while (!Serial && (millis() - serialWaitStart) < 3000) {
    delay(10);
  }

  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_SENSOR, INPUT_PULLUP);
  pinMode(PIN_BUTTON_PREV, INPUT_PULLUP);
  pinMode(PIN_BUTTON_ACTION, INPUT_PULLUP);
  pinMode(PIN_BUTTON_NEXT, INPUT_PULLUP);

  randomSeed(micros());
  Wire.begin();
  useDisplayI2cClock();
  pendingSensorLevelHigh = digitalRead(PIN_SENSOR) == HIGH;
  attachInterrupt(digitalPinToInterrupt(PIN_SENSOR), onSensorInterrupt, CHANGE);

  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS);
  display.clearDisplay();
  display.display();

  const bool eepromDetected = detectRtcStorageAddress();
  rtcClockAvailable = detectRtcClock();
  if (Serial) {
    if (!eepromDetected) {
      Serial.println("TinyRTC EEPROM not detected");
    }
    if (!rtcClockAvailable) {
      Serial.println("RTC clock not detected");
    }
  }

  lastClockTickMs = millis();
  lastDrawMs = lastClockTickMs;
  loadState(lastClockTickMs);
  pet.lastActivityMs = lastClockTickMs;

  if (pet.statusMessage[0] == '\0') {
    setStatus("HealthPet ready", millis(), 2000);
  }
}

void loop() {
  const uint32_t now = millis();

  updateClock(now);
  updateButton(prevButton, now);
  updateButton(actionButton, now);
  updateButton(nextButton, now);
  updateSensor(now);
  handleInput(now);
  updateSound(now);
  saveState(false);
  if ((now - lastDrawMs) >= DISPLAY_REFRESH_MS) {
    lastDrawMs = now;
    drawUi(now);
  }

  waitForInterrupt();
}
