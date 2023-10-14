#pragma once

#include <gui/view.h>

#include <input/input.h>
#include "jclock_settings.h"
#include <notification/notification_messages.h>

#define DEBUG_JCLOCK 1

#define TAG "jClock"

#define CLOCK_ISO_DATE_FORMAT "%.4d-%.2d-%.2d"
#define CLOCK_RFC_DATE_FORMAT "%.2d-%.2d-%.4d"
#define JCLOCK_TIME_FORMAT "%.2d:%.2d:%.2d"

#define MERIDIAN_FORMAT "%s"
#define MERIDIAN_STRING_AM "AM"
#define MERIDIAN_STRING_PM "PM"

#define TIME_LEN 12
#define DATE_LEN 14
#define MERIDIAN_LEN 3

typedef enum {
    EventTypeTick,
    EventTypeKey,
} EventType;

typedef struct {
    EventType type;
    InputEvent input;
} PluginEvent;

typedef enum { JJY_NONE = 0, JJY_AUTO_ENABLED, JJY_AUTO_TRANSMIT, JJY_FORCED } JJYmode;

typedef struct {
    NotificationApp* notifications;
    FuriMutex* mutex;
    FuriMessageQueue* event_queue;

    ClockSettings Settings;
    FuriHalRtcDateTime DateTime;
    uint32_t Timestamp;

    uint32_t TimerStartTimestamp;
    uint32_t TimerStoppedSeconds;
    bool TimerRunning;

    JJYmode JJYMode;
    FuriHalRtcDateTime JJYDateTime;     // corrected to dTMZ time
    uint32_t JJYTimestamp;              // for correction dTMZ
    uint32_t JJYInstance;               // current stage of transmitting

    float OldDisplayBrightness;
    bool isCharging;
} ClockState;
