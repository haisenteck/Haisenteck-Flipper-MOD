#pragma once

#include "jclock_settings_filename.h"

#include <furi_hal.h>
#include <stdint.h>
#include <stdbool.h>
#include <toolbox/saved_struct.h>
#include <storage/storage.h>

#define JCLOCK_SETTINGS_VER (1)
#define JCLOCK_SETTINGS_PATH EXT_PATH(JCLOCK_SETTINGS_FILE_NAME)
#define JCLOCK_SETTINGS_MAGIC (0xC1)

#define SAVE_JCLOCK_SETTINGS(x) \
    saved_struct_save(          \
        JCLOCK_SETTINGS_PATH,   \
        (x),                    \
        sizeof(ClockSettings),  \
        JCLOCK_SETTINGS_MAGIC,  \
        JCLOCK_SETTINGS_VER)

#define LOAD_JCLOCK_SETTINGS(x) \
    saved_struct_load(          \
        JCLOCK_SETTINGS_PATH,   \
        (x),                    \
        sizeof(ClockSettings),  \
        JCLOCK_SETTINGS_MAGIC,  \
        JCLOCK_SETTINGS_VER)

typedef enum {
    H12 = 1,
    H24 = 2,
} TimeFormat;

typedef enum {
    Iso = 1,    // ISO 8601: yyyy-mm-dd
    Rfc = 2,    // RFC 5322: dd-mm-yyyy
} DateFormat;

typedef struct {
    TimeFormat TimeFormat;
    DateFormat DateFormat;
    bool isJJYEnabled;
    float JJYDtz;
    bool isBacklightOnCharge;
} ClockSettings;

extern const ClockSettings defaultClockSettings;

#define JJY_DTZ_COUNT 193
extern const char* const JJYDtzText[JJY_DTZ_COUNT];
extern const float JJYDtzValue[JJY_DTZ_COUNT];

//extern uint8_t jclock_value_index_uint32(const uint32_t value, const uint32_t values[], uint8_t values_count);
//extern uint8_t jclock_value_index_float(const float value, const float values[], uint8_t values_count);

//extern int32_t jclock_settings(void* context_settings);
