#pragma once

#include "base.h"

#include "../blocks/const.h"
#include "../blocks/decoder.h"
#include "../blocks/encoder.h"
#include "../blocks/generic.h"
#include "../blocks/math.h"

#define SUBGHZ_PROTOCOL_SCHRADER_GG4_NAME "Schrader GG4"

typedef struct SubGhzProtocolDecoderSchraderGG4 SubGhzProtocolDecoderSchraderGG4;
typedef struct SubGhzProtocolEncoderSchraderGG4 SubGhzProtocolEncoderSchraderGG4;

extern const SubGhzProtocolDecoder subghz_protocol_schrader_gg4_decoder;
extern const SubGhzProtocolEncoder subghz_protocol_schrader_gg4_encoder;
extern const SubGhzProtocol subghz_protocol_schrader_gg4;

/**
 * Allocate SubGhzProtocolDecoderSchraderGG4.
 * @param environment Pointer to a SubGhzEnvironment instance
 * @return SubGhzProtocolDecoderSchraderGG4* pointer to a SubGhzProtocolDecoderSchraderGG4 instance
 */
void* subghz_protocol_decoder_schrader_gg4_alloc(SubGhzEnvironment* environment);

/**
 * Free SubGhzProtocolDecoderSchraderGG4.
 * @param context Pointer to a SubGhzProtocolDecoderSchraderGG4 instance
 */
void subghz_protocol_decoder_schrader_gg4_free(void* context);

/**
 * Reset decoder SubGhzProtocolDecoderSchraderGG4.
 * @param context Pointer to a SubGhzProtocolDecoderSchraderGG4 instance
 */
void subghz_protocol_decoder_schrader_gg4_reset(void* context);

/**
 * Parse a raw sequence of levels and durations received from the air.
 * @param context Pointer to a SubGhzProtocolDecoderSchraderGG4 instance
 * @param level Signal level true-high false-low
 * @param duration Duration of this level in, us
 */
void subghz_protocol_decoder_schrader_gg4_feed(void* context, bool level, uint32_t duration);

/**
 * Getting the hash sum of the last randomly received parcel.
 * @param context Pointer to a SubGhzProtocolDecoderSchraderGG4 instance
 * @return hash Hash sum
 */
uint8_t subghz_protocol_decoder_schrader_gg4_get_hash_data(void* context);

/**
 * Serialize data SubGhzProtocolDecoderSchraderGG4.
 * @param context Pointer to a SubGhzProtocolDecoderSchraderGG4 instance
 * @param flipper_format Pointer to a FlipperFormat instance
 * @param preset The modulation on which the signal was received, SubGhzRadioPreset
 * @return status
 */
SubGhzProtocolStatus subghz_protocol_decoder_schrader_gg4_serialize(
    void* context,
    FlipperFormat* flipper_format,
    SubGhzRadioPreset* preset);

/**
 * Deserialize data SubGhzProtocolDecoderSchraderGG4.
 * @param context Pointer to a SubGhzProtocolDecoderSchraderGG4 instance
 * @param flipper_format Pointer to a FlipperFormat instance
 * @return status
 */
SubGhzProtocolStatus
    subghz_protocol_decoder_schrader_gg4_deserialize(void* context, FlipperFormat* flipper_format);

/**
 * Getting a textual representation of the received data.
 * @param context Pointer to a SubGhzProtocolDecoderSchraderGG4 instance
 * @param output Resulting text
 */
void subghz_protocol_decoder_schrader_gg4_get_string(void* context, FuriString* output);
