/*
* jClock - basic clock with clock synchronization protocol JJY
* Author: hrundeel (https://github.com/hrundeel)
* Added:
*   Options for JJY disable/enable.
*   Options for JJY delta TZ (if your current watch region don't match flipper time).
*   Reworked getting rtc time/update local values.
*   Reworked main screen (+charge/jjy/tx status icons, +charge%%).
* Based on:
*   Simple clock for Flipper Zero by CompaqDisc (https://github.com/CompaqDisc, https://gist.github.com/CompaqDisc/4e329c501bd03c1e801849b81f48ea61)
*   Timer by GMMan (?)
*   Settings by kowalski7cc (https://github.com/kowalski7cc)
*/

#include <furi.h>
#include <furi_hal.h>

#include <furi_hal_gpio.h>
#include <furi_hal_resources.h>
#include <furi_hal_power.h>
#include <furi_hal_rtc.h>

#include <notification/notification.h>
#include <notification/notification_messages.h>
//#include <notification/notification_app.h>

#include <gui/gui.h>
#include <gui/elements.h>
//#include <gui/icon.h>
#include <lib/toolbox/value_index.h>

#include "jclock.h"
#include "jclock_settings.h"
#include "compiled/jclock_icons.h"

//JJY 0..5hrs +3MSK?
#define DTZ_LEN 9


const NotificationMessage message_force_display_brightness_setting_night = {
    .type = NotificationMessageTypeForceDisplayBrightnessSetting,
    .data.forced_settings.display_brightness = 0.1f,
};

const NotificationSequence sequence_DisplayNightMode = {
    &message_display_backlight_enforce_on,
    &message_force_display_brightness_setting_night,
    NULL,
};



// correct time to dTZ (-24f..+24f)
static void jjy_correct_dtz(ClockState* state) {
    state->JJYTimestamp = (double)furi_hal_rtc_datetime_to_timestamp(&state->DateTime) + ((double)state->Settings.JJYDtz * 86400);
    //check to crossing daylight saving time?
    //!!! add function when will be available
    //state->JJYdatetime = *localtime(&corrected_timestamp);
    return;
}

// transmit current signal to antenna
static void jjy_transmit(ClockState* state) {

    //const GpioPin gpio_nfc_irq_rfid_pull = {.port = RFID_PULL_GPIO_Port, .pin = RFID_PULL_Pin};
    //const GpioPin gpio_rfid_carrier_out = {.port = RFID_OUT_GPIO_Port, .pin = RFID_OUT_Pin};
    //const GpioPin gpio_rfid_data_in = {.port = RFID_RF_IN_GPIO_Port, .pin = RFID_RF_IN_Pin};
    //const GpioPin gpio_rfid_carrier = {.port = RFID_CARRIER_GPIO_Port, .pin = RFID_CARRIER_Pin};

        //furi_hal_gpio_write(&gpio_ext_pc0, false);
    //furi_hal_gpio_init(&gpio_ext_pa7, GpioModeOutputPushPull, GpioPullNo, GpioSpeedVeryHigh);
    jjy_correct_dtz(state);

    //пока просто помигаем
    if (state->DateTime.second & 1) {
        // release
        //furi_hal_ibutton_pin_high();
        furi_hal_gpio_write(&gpio_ext_pc0, true);
        //furi_delay_us(OWH_WRITE_1_RELEASE);
    }
    else {
        // drive low
        //furi_hal_ibutton_pin_low();
        furi_hal_gpio_write(&gpio_ext_pc0, false);
        //furi_delay_us(OWH_WRITE_1_DRIVE);
    }
    return;
}


static void jclock_input_callback(InputEvent* input_event, FuriMessageQueue* event_queue) {
    furi_assert(event_queue);
    PluginEvent event = { .type = EventTypeKey, .input = *input_event };
    furi_message_queue_put(event_queue, &event, FuriWaitForever);
}

static void jclock_render_callback(Canvas* const canvas, void* ctx) {
    //canvas_clear(canvas);
    //canvas_set_color(canvas, ColorBlack);

    ClockState* state = ctx;
    if (furi_mutex_acquire(state->mutex, 200) != FuriStatusOk) {
        //FURI_LOG_D(TAG, "Can't obtain mutex, requeue render");
        PluginEvent event = { .type = EventTypeTick };
        furi_message_queue_put(state->event_queue, &event, 0);
        return;
    }

    char TimeString[TIME_LEN];
    char DateString[DATE_LEN];
    char MeridianString[MERIDIAN_LEN];
    char TimerString[20];

    char DtzString[DTZ_LEN]; // JJY
    bool now_isCharging;

    now_isCharging = furi_hal_power_is_charging();

    if (now_isCharging != state->isCharging) {   // just changed
        state->isCharging = now_isCharging;

        // Check JJY mode
        if (state->isCharging) {    // charging now
            notification_message(state->notifications, &sequence_single_vibro);

            if (state->Settings.isBacklightOnCharge) {
                //notification_message(state->notifications, &sequence_display_backlight_enforce_on);
                notification_message(state->notifications, &sequence_DisplayNightMode);

                FURI_LOG_I(TAG, "Backlight Enforce");
            }

            // check options for auto enabled?
            if (state->JJYMode == JJY_AUTO_ENABLED) {
#if (!DEBUG_JCLOCK)
                if (state->datetime.hour < 5) {
#endif
                    state->JJYMode = JJY_AUTO_TRANSMIT;
                    FURI_LOG_I(TAG, "JJY-A Tx (charge on");
#if (!DEBUG_JCLOCK)
                }
#endif
            }

        }
        else { // not charging (just now?)
            notification_message(state->notifications, &sequence_display_backlight_enforce_auto);
            FURI_LOG_I(TAG, "Backlight Auto");

            if (state->JJYMode == JJY_AUTO_TRANSMIT) {
                if (state->Settings.isJJYEnabled) {
                    state->JJYMode = JJY_AUTO_ENABLED; // check if options ok
                    FURI_LOG_I(TAG, "JJY-A (charge off");
                }
            }
            /*
                        else if (state->JJYMode != JJY_FORCED) {
                            state->JJYMode = JJY_NONE; // check if options ok
                            FURI_LOG_I(TAG, "JJY disabled");
                        }
            */
        }
    }

    // secondary clock functions (display clock/timer)
    if (state->Settings.TimeFormat == H24) {
        snprintf(
            TimeString,
            TIME_LEN,
            JCLOCK_TIME_FORMAT,
            state->DateTime.hour,
            state->DateTime.minute,
            state->DateTime.second);
    }
    else {
        bool pm = state->DateTime.hour > 12;
        bool pm12 = state->DateTime.hour >= 12;
        snprintf(
            TimeString,
            TIME_LEN,
            JCLOCK_TIME_FORMAT,
            pm ? state->DateTime.hour - 12 : state->DateTime.hour,
            state->DateTime.minute,
            state->DateTime.second);

        snprintf(
            MeridianString,
            MERIDIAN_LEN,
            MERIDIAN_FORMAT,
            pm12 ? MERIDIAN_STRING_PM : MERIDIAN_STRING_AM);
    }

    if (state->Settings.DateFormat == Iso) {
        snprintf(
            DateString, DATE_LEN, CLOCK_ISO_DATE_FORMAT, state->DateTime.year, state->DateTime.month, state->DateTime.day);
    }
    else {
        snprintf(
            DateString, DATE_LEN, CLOCK_RFC_DATE_FORMAT, state->DateTime.day, state->DateTime.month, state->DateTime.year);
    }

    bool timer_running = state->TimerRunning;
    uint32_t timer_start_timestamp = state->TimerStartTimestamp;
    uint32_t timer_stopped_seconds = state->TimerStoppedSeconds;

    furi_mutex_release(state->mutex);

    canvas_set_font(canvas, FontBigNumbers);

    if (timer_start_timestamp != 0) {   // timer

        int32_t elapsed_secs = timer_running ? (state->Timestamp - timer_start_timestamp) :
            timer_stopped_seconds;
        snprintf(TimerString, 20, "%.2ld:%.2ld", elapsed_secs / 60, elapsed_secs % 60);
        canvas_draw_str_aligned(canvas, 64, 8, AlignCenter, AlignCenter, TimeString); // DRAW TIME
        canvas_draw_str_aligned(canvas, 64, 32, AlignCenter, AlignTop, TimerString); // DRAW TIMER
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str_aligned(canvas, 64, 20, AlignCenter, AlignTop, DateString); // DRAW DATE
        elements_button_left(canvas, "Reset");

    }
    else {  // main

        canvas_draw_str_aligned(canvas, 64, 28, AlignCenter, AlignCenter, TimeString);
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str_aligned(canvas, 64, 42, AlignCenter, AlignTop, DateString);

        if (state->Settings.TimeFormat == H12)
            canvas_draw_str_aligned(canvas, 65, 12, AlignCenter, AlignCenter, MeridianString);

        // Battery% / Battery charge
        snprintf(DtzString, DTZ_LEN, "%02d", furi_hal_power_get_pct());
        canvas_draw_str_aligned(canvas, 120, 0, AlignRight, AlignTop, DtzString); // DRAW dTZ
        if (state->isCharging) {
            canvas_draw_icon(canvas, 121, 0, &I_JJY_Charge_7x7);
        }
        else {
            canvas_draw_icon(canvas, 121, 0, &I_JJY_NoCharge_7x7);
        }


        // JJY indication
        switch (state->JJYMode) {
        case JJY_AUTO_ENABLED:
            //canvas_draw_icon(canvas, canvas_width(canvas) - icon_get_width(&I_jjy_auto_7px), 0, &I_jjy_auto_7px);
            canvas_draw_icon(canvas, 0, 0, &I_JJY_Auto_10x7);
            break;

        case JJY_AUTO_TRANSMIT:
            //canvas_draw_icon(canvas, canvas_width(canvas) - icon_get_width(&I_jjy_auto_transmit_7px), 0, &I_jjy_auto_transmit_7px);
            canvas_draw_icon(canvas, 0, 0, &I_JJY_AutoTransmit_10x7);
            break;

        case JJY_FORCED:
            //canvas_draw_icon(canvas, canvas_width(canvas) - icon_get_width(&I_jjy_forced_7px), 0, &I_jjy_forced7px);
            canvas_draw_icon(canvas, 0, 0, &I_JJY_Forced_10x7);
            break;

        case JJY_NONE:
        default:
            break;
        }

        // JJY dTZ
        if (state->JJYMode != JJY_NONE) {
            uint8_t  ValueIndex = value_index_float((float)state->Settings.JJYDtz, JJYDtzValue, JJY_DTZ_COUNT);
            //FURI_LOG_I(TAG, "JJY F dTZ: %s", JJYDtzText[ValueIndex]);
            snprintf(DtzString, DTZ_LEN, "%s", JJYDtzText[ValueIndex]);
            canvas_set_font(canvas, FontSecondary);
            canvas_draw_str_aligned(canvas, 12, 0, AlignLeft, AlignTop, DtzString); // DRAW dTZ
        }

    }

    if (timer_running) {
        elements_button_center(canvas, "Stop");
    }
    else if (timer_start_timestamp != 0 && !timer_running) {
        elements_button_center(canvas, "Start");
    }

}

static void jclock_state_init(ClockState* const state) {
    if (!LOAD_JCLOCK_SETTINGS(&state->Settings)) {
        state->Settings = defaultClockSettings;      //defaults
        SAVE_JCLOCK_SETTINGS(&state->Settings);
    }

    if (state->Settings.TimeFormat != H12 && state->Settings.TimeFormat != H24) {
        state->Settings.TimeFormat = H12;
    }
    if (state->Settings.DateFormat != Iso && state->Settings.DateFormat != Rfc) {
        state->Settings.DateFormat = Iso;
    }
    FURI_LOG_D(TAG, "Time format: %s", state->Settings.TimeFormat == H12 ? "12h" : "24h");
    FURI_LOG_D(TAG, "Date format: %s", state->Settings.DateFormat == Iso ? "ISO 8601" : "RFC 5322");
    //furi_hal_rtc_get_datetime(&state->datetime);

    //JJY
    FURI_LOG_I(TAG, "JJY Enabled: %s", state->Settings.isJJYEnabled ? "ON" : "OFF");
    if (state->Settings.isJJYEnabled) {
        state->JJYMode = JJY_AUTO_ENABLED;
    }
    else {
        state->JJYMode = JJY_NONE;
    }
}

// Runs every 1000ms by default
static void jclock_tick(void* ctx) {
    furi_assert(ctx);
    FuriMessageQueue* event_queue = ctx;
    PluginEvent event = { .type = EventTypeTick };
    // It's OK to loose this event if system overloaded
    furi_message_queue_put(event_queue, &event, 0);
}

int32_t jclock(void* p) {
    UNUSED(p);
    ClockState* plugin_state = malloc(sizeof(ClockState));

    plugin_state->event_queue = furi_message_queue_alloc(8, sizeof(PluginEvent));
    if (plugin_state->event_queue == NULL) {
        FURI_LOG_E(TAG, "Cannot create event queue");
        free(plugin_state);
        return 255;
    }
    //FURI_LOG_D(TAG, "Event queue created");

    plugin_state->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    if (plugin_state->mutex == NULL) {
        FURI_LOG_E(TAG, "Cannot create mutex");
        furi_message_queue_free(plugin_state->event_queue);
        free(plugin_state);
        return 255;
    }
    //FURI_LOG_D(TAG, "Mutex created");

    jclock_state_init(plugin_state);

    // Set system callbacks
    ViewPort* view_port = view_port_alloc();
    view_port_draw_callback_set(view_port, jclock_render_callback, plugin_state);
    view_port_input_callback_set(view_port, jclock_input_callback, plugin_state->event_queue);

    FuriTimer* timer =
        furi_timer_alloc(jclock_tick, FuriTimerTypePeriodic, plugin_state->event_queue);

    if (timer == NULL) {
        FURI_LOG_E(TAG, "Cannot create timer");
        furi_mutex_free(plugin_state->mutex);
        furi_message_queue_free(plugin_state->event_queue);
        free(plugin_state);
        return 255;
    }
    //FURI_LOG_D(TAG, "Timer created");

    // Open GUI and register view_port
    Gui* gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);

    furi_timer_start(timer, furi_kernel_get_tick_frequency());
    //FURI_LOG_D(TAG, "Timer started");

    furi_hal_rtc_get_datetime(&plugin_state->DateTime);
    plugin_state->Timestamp = furi_hal_rtc_datetime_to_timestamp(&plugin_state->DateTime);

    // Open Notification record
    plugin_state->notifications = furi_record_open(RECORD_NOTIFICATION);

    // State of charging
    plugin_state->isCharging = furi_hal_power_is_charging();

    // Backlight On Charged
    if ((plugin_state->Settings.isBacklightOnCharge) && (plugin_state->isCharging)) {
        //notification_message(plugin_state->notifications, &sequence_display_backlight_enforce_on);
        notification_message(plugin_state->notifications, &sequence_DisplayNightMode);
        FURI_LOG_I(TAG, "Backlight Enforce");
    }
    else {
        FURI_LOG_I(TAG, "Backlight Auto");
    }

    //JJY init
    //furi_hal_gpio_init_simple(&gpio_ext_pc0, GpioModeOutputPushPull);
    furi_hal_gpio_init(&gpio_ext_pc0, GpioModeOutputPushPull, GpioPullNo, GpioSpeedVeryHigh);


    // Main loop
    PluginEvent event;
    for (bool processing = true; processing;) {
        FuriStatus event_status = furi_message_queue_get(plugin_state->event_queue, &event, 100);

        if (event_status != FuriStatusOk) continue;

        if (furi_mutex_acquire(plugin_state->mutex, FuriWaitForever) != FuriStatusOk) continue;

        // press events
        if (event.type == EventTypeKey) {
            //if (event.input.type == InputTypeShort || event.input.type == InputTypeRepeat) {
            if (event.input.type == InputTypeShort) {
                switch (event.input.key) {

                case InputKeyUp:
                    //plugin_state->jjy_forced = true;
                    plugin_state->JJYMode = JJY_FORCED;
                    FURI_LOG_I(TAG, "JJY-F Tx");
                    break;

                case InputKeyDown:
                    // check enabled/time ok to transmit!
                    switch (plugin_state->JJYMode) {
                    case JJY_AUTO_ENABLED:
                        plugin_state->JJYMode = JJY_NONE;
                        FURI_LOG_I(TAG, "JJY none");
                        break;

                    case JJY_AUTO_TRANSMIT:
                        plugin_state->JJYMode = JJY_AUTO_ENABLED;
                        FURI_LOG_I(TAG, "JJY-A");
                        break;

                    case JJY_FORCED:
                        if (plugin_state->Settings.isJJYEnabled) {
                            plugin_state->JJYMode = JJY_AUTO_ENABLED;
                            FURI_LOG_I(TAG, "JJY-A");
                        }
                        else {
                            plugin_state->JJYMode = JJY_NONE;
                            FURI_LOG_I(TAG, "JJY none ");
                        }
                        break;

                    case JJY_NONE:
                    default:
                        break;
                    }

                    break;

                case InputKeyRight:
                    break;

                case InputKeyLeft:
                    if (plugin_state->TimerStartTimestamp != 0) {
                        // Reset seconds
                        plugin_state->TimerRunning = false;
                        plugin_state->TimerStartTimestamp = 0;
                        plugin_state->TimerStoppedSeconds = 0;
                    }
                    break;


                case InputKeyOk:;
                    // START/STOP TIMER

                    FuriHalRtcDateTime curr_dt;
                    furi_hal_rtc_get_datetime(&curr_dt);
                    uint32_t curr_ts = furi_hal_rtc_datetime_to_timestamp(&curr_dt);

                    if (plugin_state->TimerRunning) {
                        // Update stopped seconds
                        plugin_state->TimerStoppedSeconds =
                            curr_ts - plugin_state->TimerStartTimestamp;
                    }
                    else {
                        if (plugin_state->TimerStartTimestamp == 0) {
                            // Set starting timestamp if this is first time
                            plugin_state->TimerStartTimestamp = curr_ts;
                        }
                        else {
                            // Timer was already running, need to slightly readjust so we don't
                            // count the intervening time
                            plugin_state->TimerStartTimestamp =
                                curr_ts - plugin_state->TimerStoppedSeconds;
                        }
                    }
                    plugin_state->TimerRunning = !plugin_state->TimerRunning;

                    //jclock_settings(plugin_state);
                    break;

                case InputKeyBack:
                    // Exit the plugin
                    processing = false;
                    break;

                default:
                    break;
                }
            }
            else if (event.input.type == InputTypeLong) {      // LONG PRESS
                switch (event.input.key) {
                case InputKeyOk:
                    //jclock_settings(plugin_state);
                    break;
                default:
                    break;
                }
            }
        }
        else if (event.type == EventTypeTick) {
            furi_hal_rtc_get_datetime(&plugin_state->DateTime);
            plugin_state->Timestamp = furi_hal_rtc_datetime_to_timestamp(&plugin_state->DateTime);

            //JJY transmission if enabled
            if ((plugin_state->JJYMode == JJY_AUTO_TRANSMIT) || (plugin_state->JJYMode == JJY_FORCED)) {
                jjy_transmit(plugin_state);
            }

        }
        view_port_update(view_port);
        furi_mutex_release(plugin_state->mutex);
    }

    // JJY free
    // Reset GPIO pins to default state
    furi_hal_gpio_init(&gpio_ext_pc0, GpioModeAnalog, GpioPullNo, GpioSpeedLow);

    // Backlight to auto default
    notification_message(plugin_state->notifications, &sequence_display_backlight_enforce_auto);

    furi_timer_free(timer);
    view_port_enabled_set(view_port, false);
    gui_remove_view_port(gui, view_port);
    furi_record_close(RECORD_GUI);
    furi_record_close(RECORD_NOTIFICATION);
    view_port_free(view_port);
    furi_message_queue_free(plugin_state->event_queue);
    furi_mutex_free(plugin_state->mutex);
    free(plugin_state);

    return 0;
}
