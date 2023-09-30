#pragma once

#include <lib/subghz/protocols/base.h>

#include <lib/subghz/blocks/const.h>
#include <lib/subghz/blocks/decoder.h>
#include <lib/subghz/blocks/encoder.h>
#include <lib/subghz/blocks/generic.h>
#include <lib/subghz/blocks/math.h>

#define SUBGHZ_PROTOCOL_OREGON_V1_NAME "Oregon-v1"

typedef struct subghz_protocol_decoder_oregon_v1 subghz_protocol_decoder_oregon_v1;
typedef struct subghz_protocol_encoder_oregon_v1 subghz_protocol_encoder_oregon_v1;

extern const SubGhzProtocolDecoder subghz_protocol_oregon_v1_decoder;
extern const SubGhzProtocolEncoder subghz_protocol_oregon_v1_encoder;
extern const SubGhzProtocol subghz_protocol_oregon_v1;

/**
 * Allocate subghz_protocol_decoder_oregon_v1.
 * @param environment Pointer to a SubGhzEnvironment instance
 * @return subghz_protocol_decoder_oregon_v1* pointer to a subghz_protocol_decoder_oregon_v1 instance
 */
void* subghz_protocol_decoder_oregon_v1_alloc(SubGhzEnvironment* environment);

/**
 * Free subghz_protocol_decoder_oregon_v1.
 * @param context Pointer to a subghz_protocol_decoder_oregon_v1 instance
 */
void subghz_protocol_decoder_oregon_v1_free(void* context);

/**
 * Reset decoder subghz_protocol_decoder_oregon_v1.
 * @param context Pointer to a subghz_protocol_decoder_oregon_v1 instance
 */
void subghz_protocol_decoder_oregon_v1_reset(void* context);

/**
 * Parse a raw sequence of levels and durations received from the air.
 * @param context Pointer to a subghz_protocol_decoder_oregon_v1 instance
 * @param level Signal level true-high false-low
 * @param duration Duration of this level in, us
 */
void subghz_protocol_decoder_oregon_v1_feed(void* context, bool level, uint32_t duration);

/**
 * Getting the hash sum of the last randomly received parcel.
 * @param context Pointer to a subghz_protocol_decoder_oregon_v1 instance
 * @return hash Hash sum
 */
uint8_t subghz_protocol_decoder_oregon_v1_get_hash_data(void* context);

/**
 * Serialize data subghz_protocol_decoder_oregon_v1.
 * @param context Pointer to a subghz_protocol_decoder_oregon_v1 instance
 * @param flipper_format Pointer to a FlipperFormat instance
 * @param preset The modulation on which the signal was received, SubGhzRadioPreset
 * @return status
 */
SubGhzProtocolStatus subghz_protocol_decoder_oregon_v1_serialize(
    void* context,
    FlipperFormat* flipper_format,
    SubGhzRadioPreset* preset);

/**
 * Deserialize data subghz_protocol_decoder_oregon_v1.
 * @param context Pointer to a subghz_protocol_decoder_oregon_v1 instance
 * @param flipper_format Pointer to a FlipperFormat instance
 * @return status
 */
SubGhzProtocolStatus subghz_protocol_decoder_oregon_v1_deserialize(void* context, FlipperFormat* flipper_format);

/**
 * Getting a textual representation of the received data.
 * @param context Pointer to a subghz_protocol_decoder_oregon_v1 instance
 * @param output Resulting text
 */
void subghz_protocol_decoder_oregon_v1_get_string(void* context, FuriString* output);


void subghz_protocol_encoder_oregon_v1_stop(void* context);

SubGhzProtocolStatus subghz_protocol_encoder_oregon_v1_deserialize(void* context, FlipperFormat* flipper_format);

void subghz_protocol_encoder_oregon_v1_free(void* context);

LevelDuration subghz_protocol_encoder_oregon_v1_yield(void* context);

void* subghz_protocol_encoder_oregon_v1_alloc(SubGhzEnvironment* environment);