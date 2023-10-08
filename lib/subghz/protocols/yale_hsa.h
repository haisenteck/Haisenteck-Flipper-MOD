#pragma once

#include "base.h"

#define SUBGHZ_PROTOCOL_YALE_HSA_NAME "Yale-HSA"

typedef struct SubGhzProtocolDecoderyale_hsa SubGhzProtocolDecoderyale_hsa;
typedef struct SubGhzProtocolEncoderyale_hsa SubGhzProtocolEncoderyale_hsa;

extern const SubGhzProtocolDecoder subghz_protocol_yale_hsa_decoder;
extern const SubGhzProtocolEncoder subghz_protocol_yale_hsa_encoder;
extern const SubGhzProtocol subghz_protocol_yale_hsa;

/**
 * Allocate SubGhzProtocolEncoderyale_hsa.
 * @param environment Pointer to a SubGhzEnvironment instance
 * @return SubGhzProtocolEncoderyale_hsa* pointer to a SubGhzProtocolEncoderyale_hsa instance
 */
void* subghz_protocol_encoder_yale_hsa_alloc(SubGhzEnvironment* environment);

/**
 * Free SubGhzProtocolEncoderyale_hsa.
 * @param context Pointer to a SubGhzProtocolEncoderyale_hsa instance
 */
void subghz_protocol_encoder_yale_hsa_free(void* context);

/**
 * Deserialize and generate an upload to send.
 * @param context Pointer to a SubGhzProtocolEncoderyale_hsa instance
 * @param flipper_format Pointer to a FlipperFormat instance
 * @return status
 */
SubGhzProtocolStatus
    subghz_protocol_encoder_yale_hsa_deserialize(void* context, FlipperFormat* flipper_format);

/**
 * Forced transmission stop.
 * @param context Pointer to a SubGhzProtocolEncoderyale_hsa instance
 */
void subghz_protocol_encoder_yale_hsa_stop(void* context);

/**
 * Getting the level and duration of the upload to be loaded into DMA.
 * @param context Pointer to a SubGhzProtocolEncoderyale_hsa instance
 * @return LevelDuration
 */
LevelDuration subghz_protocol_encoder_yale_hsa_yield(void* context);

/**
 * Allocate SubGhzProtocolDecoderyale_hsa.
 * @param environment Pointer to a SubGhzEnvironment instance
 * @return SubGhzProtocolDecoderyale_hsa* pointer to a SubGhzProtocolDecoderyale_hsa instance
 */
void* subghz_protocol_decoder_yale_hsa_alloc(SubGhzEnvironment* environment);

/**
 * Free SubGhzProtocolDecoderyale_hsa.
 * @param context Pointer to a SubGhzProtocolDecoderyale_hsa instance
 */
void subghz_protocol_decoder_yale_hsa_free(void* context);

/**
 * Reset decoder SubGhzProtocolDecoderyale_hsa.
 * @param context Pointer to a SubGhzProtocolDecoderyale_hsa instance
 */
void subghz_protocol_decoder_yale_hsa_reset(void* context);

/**
 * Parse a raw sequence of levels and durations received from the air.
 * @param context Pointer to a SubGhzProtocolDecoderyale_hsa instance
 * @param level Signal level true-high false-low
 * @param duration Duration of this level in, us
 */
void subghz_protocol_decoder_yale_hsa_feed(void* context, bool level, uint32_t duration);

/**
 * Getting the hash sum of the last randomly received parcel.
 * @param context Pointer to a SubGhzProtocolDecoderyale_hsa instance
 * @return hash Hash sum
 */
uint8_t subghz_protocol_decoder_yale_hsa_get_hash_data(void* context);

/**
 * Serialize data SubGhzProtocolDecoderyale_hsa.
 * @param context Pointer to a SubGhzProtocolDecoderyale_hsa instance
 * @param flipper_format Pointer to a FlipperFormat instance
 * @param preset The modulation on which the signal was received, SubGhzRadioPreset
 * @return status
 */
SubGhzProtocolStatus subghz_protocol_decoder_yale_hsa_serialize(
    void* context,
    FlipperFormat* flipper_format,
    SubGhzRadioPreset* preset);

/**
 * Deserialize data SubGhzProtocolDecoderyale_hsa.
 * @param context Pointer to a SubGhzProtocolDecoderyale_hsa instance
 * @param flipper_format Pointer to a FlipperFormat instance
 * @return status
 */
SubGhzProtocolStatus
    subghz_protocol_decoder_yale_hsa_deserialize(void* context, FlipperFormat* flipper_format);

/**
 * Getting a textual representation of the received data.
 * @param context Pointer to a SubGhzProtocolDecoderyale_hsa instance
 * @param output Resulting text
 */
void subghz_protocol_decoder_yale_hsa_get_string(void* context, FuriString* output);
