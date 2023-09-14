#pragma once

#include "base.h"

#include "../blocks/const.h"
#include "../blocks/decoder.h"
#include "../blocks/encoder.h"
#include "../blocks/generic.h"
#include "../blocks/math.h"

#define subghz_protocol_AMBIENT_WEATHER_NAME "Ambient_Weather"

typedef struct subghz_protocol_DecoderAmbient_Weather subghz_protocol_DecoderAmbient_Weather;
typedef struct subghz_protocol_EncoderAmbient_Weather subghz_protocol_EncoderAmbient_Weather;

extern const SubGhzProtocolDecoder subghz_protocol_ambient_weather_decoder;
extern const SubGhzProtocolEncoder subghz_protocol_ambient_weather_encoder;
extern const SubGhzProtocol subghz_protocol_ambient_weather;

/**
 * Allocate subghz_protocol_DecoderAmbient_Weather.
 * @param environment Pointer to a SubGhzEnvironment instance
 * @return subghz_protocol_DecoderAmbient_Weather* pointer to a subghz_protocol_DecoderAmbient_Weather instance
 */
void* subghz_protocol_decoder_ambient_weather_alloc(SubGhzEnvironment* environment);

/**
 * Free subghz_protocol_DecoderAmbient_Weather.
 * @param context Pointer to a subghz_protocol_DecoderAmbient_Weather instance
 */
void subghz_protocol_decoder_ambient_weather_free(void* context);

/**
 * Reset decoder subghz_protocol_DecoderAmbient_Weather.
 * @param context Pointer to a subghz_protocol_DecoderAmbient_Weather instance
 */
void subghz_protocol_decoder_ambient_weather_reset(void* context);

/**
 * Parse a raw sequence of levels and durations received from the air.
 * @param context Pointer to a subghz_protocol_DecoderAmbient_Weather instance
 * @param level Signal level true-high false-low
 * @param duration Duration of this level in, us
 */
void subghz_protocol_decoder_ambient_weather_feed(void* context, bool level, uint32_t duration);

/**
 * Getting the hash sum of the last randomly received parcel.
 * @param context Pointer to a subghz_protocol_DecoderAmbient_Weather instance
 * @return hash Hash sum
 */
uint8_t subghz_protocol_decoder_ambient_weather_get_hash_data(void* context);

/**
 * Serialize data subghz_protocol_DecoderAmbient_Weather.
 * @param context Pointer to a subghz_protocol_DecoderAmbient_Weather instance
 * @param flipper_format Pointer to a FlipperFormat instance
 * @param preset The modulation on which the signal was received, SubGhzRadioPreset
 * @return status
 */
SubGhzProtocolStatus subghz_protocol_decoder_ambient_weather_serialize(
    void* context,
    FlipperFormat* flipper_format,
    SubGhzRadioPreset* preset);

/**
 * Deserialize data subghz_protocol_DecoderAmbient_Weather.
 * @param context Pointer to a subghz_protocol_DecoderAmbient_Weather instance
 * @param flipper_format Pointer to a FlipperFormat instance
 * @return status
 */
SubGhzProtocolStatus
    subghz_protocol_decoder_ambient_weather_deserialize(void* context, FlipperFormat* flipper_format);

/**
 * Getting a textual representation of the received data.
 * @param context Pointer to a subghz_protocol_DecoderAmbient_Weather instance
 * @param output Resulting text
 */
void subghz_protocol_decoder_ambient_weather_get_string(void* context, FuriString* output);
