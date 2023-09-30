#pragma once

#include "base.h"

#define SUBGHZ_PROTOCOL_X10_NAME "X10"

typedef struct subghz_protocol_decoder_x10 subghz_protocol_decoder_x10;
typedef struct subghz_protocol_encoder_x10 subghz_protocol_encoder_x10;

extern const SubGhzProtocolDecoder subghz_protocol_x10_decoder;
extern const SubGhzProtocolEncoder subghz_protocol_x10_encoder;
extern const SubGhzProtocol subghz_protocol_x10;

/**
 * Allocate subghz_protocol_decoder_x10.
 * @param environment Pointer to a SubGhzEnvironment instance
 * @return SubGhzProtocolDecoderFaacSLH* pointer to a subghz_protocol_decoder_x10 instance
 */
void* subghz_protocol_decoder_x10_alloc(SubGhzEnvironment* environment);

/**
 * Free subghz_protocol_decoder_x10.
 * @param context Pointer to a subghz_protocol_decoder_x10 instance
 */
void subghz_protocol_decoder_x10_free(void* context);

/**
 * Reset decoder subghz_protocol_decoder_x10.
 * @param context Pointer to a subghz_protocol_decoder_x10 instance
 */
void subghz_protocol_decoder_x10_reset(void* context);

/**
 * Parse a raw sequence of levels and durations received from the air.
 * @param context Pointer to a subghz_protocol_decoder_x10 instance
 * @param level Signal level true-high false-low
 * @param duration Duration of this level in, us
 */
void subghz_protocol_decoder_x10_feed(void* context, bool level, uint32_t duration);

/**
 * Validates if the current data is valid.
 * 
 * @param decoder Pointer to a subghz_protocol_decoder_x10 instance.
 * @return true if packet data is valid
 * @return false if packet data in not valid
 */
bool subghz_protocol_x10_validate(void* context);

/**
 * Getting the hash sum of the last randomly received parcel.
 * @param context Pointer to a SubGhzProtocolDecoderFaacSLH instance
 * @return hash Hash sum
 */
uint8_t subghz_protocol_decoder_x10_get_hash_data(void* context);

/**
 * Serialize data subghz_protocol_decoder_x10.
 * @param context Pointer to a subghz_protocol_decoder_x10 instance
 * @param flipper_format Pointer to a FlipperFormat instance
 * @param preset The modulation on which the signal was received, SubGhzRadioPreset
 * @return SubGhzProtocolStatus
 */
SubGhzProtocolStatus subghz_protocol_decoder_x10_serialize(
    void* context,
    FlipperFormat* flipper_format,
    SubGhzRadioPreset* preset);

/**
 * Deserialize data subghz_protocol_decoder_x10.
 * @param context Pointer to a subghz_protocol_decoder_x10 instance
 * @param flipper_format Pointer to a FlipperFormat instance
 * @return SubGhzProtocolStatus
 */
SubGhzProtocolStatus
    subghz_protocol_decoder_x10_deserialize(void* context, FlipperFormat* flipper_format);

/**
 * Getting a textual representation of the received data.
 * @param context Pointer to a subghz_protocol_decoder_x10 instance
 * @param output Resulting text
 */
void subghz_protocol_decoder_x10_get_string(void* context, FuriString* output);


void subghz_protocol_encoder_x10_stop(void* context);

SubGhzProtocolStatus subghz_protocol_encoder_x10_deserialize(void* context, FlipperFormat* flipper_format);

void subghz_protocol_encoder_x10_free(void* context);

LevelDuration subghz_protocol_encoder_x10_yield(void* context);

void* subghz_protocol_encoder_x10_alloc(SubGhzEnvironment* environment);