#pragma once

#include "base.h"

#define SUBGHZ_PROTOCOL_BALDR_RAIN_NAME "BALDR RAIN"

typedef struct SubGhzProtocolDecoder_baldr_rain SubGhzProtocolDecoder_baldr_rain;
typedef struct SubGhzProtocolEncoder_baldr_rain SubGhzProtocolEncoder_baldr_rain;

extern const SubGhzProtocolDecoder subghz_protocol_decoder_baldr_rain_decoder;
extern const SubGhzProtocolEncoder subghz_protocol_decoder_baldr_rain_encoder;
extern const SubGhzProtocol subghz_protocol_baldr_rain;

/**
 * Allocate SubGhzProtocolEncoder_baldr_rain.
 * @param environment Pointer to a SubGhzEnvironment instance
 * @return SubGhzProtocolEncoder_baldr_rain* pointer to a SubGhzProtocolEncoder_baldr_rain instance
 */
void* subghz_protocol_encoder_baldr_rain_alloc(SubGhzEnvironment* environment);

/**
 * Free SubGhzProtocolEncoder_baldr_rain.
 * @param context Pointer to a SubGhzProtocolEncoder_baldr_rain instance
 */
void subghz_protocol_encoder_baldr_rain_free(void* context);

/**
 * Deserialize and generating an upload to send.
 * @param context Pointer to a SubGhzProtocolEncoder_baldr_rain instance
 * @param flipper_format Pointer to a FlipperFormat instance
 * @return true On success
 */
SubGhzProtocolStatus subghz_protocol_encoder_baldr_rain_deserialize(void* context, FlipperFormat* flipper_format);

/**
 * Forced transmission stop.
 * @param context Pointer to a SubGhzProtocolEncoder_baldr_rain instance
 */
void subghz_protocol_encoder_baldr_rain_stop(void* context);

/**
 * Getting the level and duration of the upload to be loaded into DMA.
 * @param context Pointer to a SubGhzProtocolEncoder_baldr_rain instance
 * @return LevelDuration 
 */
LevelDuration subghz_protocol_encoder_baldr_rain_yield(void* context);

/**
 * Allocate SubGhzProtocolDecoder_baldr_rain.
 * @param environment Pointer to a SubGhzEnvironment instance
 * @return SubGhzProtocolDecoder_baldr_rain* pointer to a SubGhzProtocolDecoder_baldr_rain instance
 */
void* subghz_protocol_decoder_baldr_rain_alloc(SubGhzEnvironment* environment);

/**
 * Free SubGhzProtocolDecoder_baldr_rain.
 * @param context Pointer to a SubGhzProtocolDecoder_baldr_rain instance
 */
void subghz_protocol_decoder_baldr_rain_free(void* context);

/**
 * Reset decoder SubGhzProtocolDecoder_baldr_rain.
 * @param context Pointer to a SubGhzProtocolDecoder_baldr_rain instance
 */
void subghz_protocol_decoder_baldr_rain_reset(void* context);

/**
 * Parse a raw sequence of levels and durations received from the air.
 * @param context Pointer to a SubGhzProtocolDecoder_baldr_rain instance
 * @param level Signal level true-high false-low
 * @param duration Duration of this level in, us
 */
void subghz_protocol_decoder_baldr_rain_feed(void* context, bool level, uint32_t duration);

/**
 * Getting the hash sum of the last randomly received parcel.
 * @param context Pointer to a SubGhzProtocolDecoder_baldr_rain instance
 * @return hash Hash sum
 */
uint8_t subghz_protocol_decoder_baldr_rain_get_hash_data(void* context);

/**
 * Serialize data SubGhzProtocolDecoder_baldr_rain.
 * @param context Pointer to a SubGhzProtocolDecoder_baldr_rain instance
 * @param flipper_format Pointer to a FlipperFormat instance
 * @param preset The modulation on which the signal was received, SubGhzRadioPreset
 * @return true On success
 */
SubGhzProtocolStatus subghz_protocol_decoder_baldr_rain_serialize(void* context, FlipperFormat* flipper_format, SubGhzRadioPreset* preset);

/**
 * Deserialize data SubGhzProtocolDecoder_baldr_rain.
 * @param context Pointer to a SubGhzProtocolDecoder_baldr_rain instance
 * @param flipper_format Pointer to a FlipperFormat instance
 * @return true On success
 */
SubGhzProtocolStatus subghz_protocol_decoder_baldr_rain_deserialize(void* context, FlipperFormat* flipper_format);

/**
 * Getting a textual representation of the received data.
 * @param context Pointer to a SubGhzProtocolDecoder_baldr_rain instance
 * @param output Resulting text
 */
void subghz_protocol_decoder_baldr_rain_get_string(void* context, FuriString* output);
