#pragma once

#include "base.h"

#include "../blocks/const.h"
#include "../blocks/decoder.h"
#include "../blocks/encoder.h"
#include "../blocks/generic.h"
#include "../blocks/math.h"

#define SUBGHZ_PROTOCOL_ACURITE_592TXR_NAME "Acurite 592TXR"

typedef struct subghz_protocolDecoderAcurite_592TXR subghz_protocolDecoderAcurite_592TXR;
typedef struct subghz_protocolEncoderAcurite_592TXR subghz_protocolEncoderAcurite_592TXR;

extern const SubGhzProtocolDecoder subghz_protocol_acurite_592txr_decoder;
extern const SubGhzProtocolEncoder subghz_protocol_acurite_592txr_encoder;
extern const SubGhzProtocol subghz_protocol_acurite_592txr;

/**
 * Allocate subghz_protocolDecoderAcurite_592TXR.
 * @param environment Pointer to a SubGhzEnvironment instance
 * @return subghz_protocolDecoderAcurite_592TXR* pointer to a subghz_protocolDecoderAcurite_592TXR instance
 */
void* subghz_protocol_decoder_acurite_592txr_alloc(SubGhzEnvironment* environment);

/**
 * Free subghz_protocolDecoderAcurite_592TXR.
 * @param context Pointer to a subghz_protocolDecoderAcurite_592TXR instance
 */
void subghz_protocol_decoder_acurite_592txr_free(void* context);

/**
 * Reset decoder subghz_protocolDecoderAcurite_592TXR.
 * @param context Pointer to a subghz_protocolDecoderAcurite_592TXR instance
 */
void subghz_protocol_decoder_acurite_592txr_reset(void* context);

/**
 * Parse a raw sequence of levels and durations received from the air.
 * @param context Pointer to a subghz_protocolDecoderAcurite_592TXR instance
 * @param level Signal level true-high false-low
 * @param duration Duration of this level in, us
 */
void subghz_protocol_decoder_acurite_592txr_feed(void* context, bool level, uint32_t duration);

/**
 * Getting the hash sum of the last randomly received parcel.
 * @param context Pointer to a subghz_protocolDecoderAcurite_592TXR instance
 * @return hash Hash sum
 */
uint8_t subghz_protocol_decoder_acurite_592txr_get_hash_data(void* context);

/**
 * Serialize data subghz_protocolDecoderAcurite_592TXR.
 * @param context Pointer to a subghz_protocolDecoderAcurite_592TXR instance
 * @param flipper_format Pointer to a FlipperFormat instance
 * @param preset The modulation on which the signal was received, SubGhzRadioPreset
 * @return status
 */
SubGhzProtocolStatus subghz_protocol_decoder_acurite_592txr_serialize(void* context, FlipperFormat* flipper_format, SubGhzRadioPreset* preset);

/**
 * Deserialize data subghz_protocolDecoderAcurite_592TXR.
 * @param context Pointer to a subghz_protocolDecoderAcurite_592TXR instance
 * @param flipper_format Pointer to a FlipperFormat instance
 * @return status
 */
SubGhzProtocolStatus subghz_protocol_decoder_acurite_592txr_deserialize(void* context, FlipperFormat* flipper_format);

/**
 * Getting a textual representation of the received data.
 * @param context Pointer to a subghz_protocolDecoderAcurite_592TXR instance
 * @param output Resulting text
 */
void subghz_protocol_decoder_acurite_592txr_get_string(void* context, FuriString* output);

void subghz_protocol_encoder_acurite_592txr_stop(void* context);

SubGhzProtocolStatus subghz_protocol_encoder_acurite_592txr_deserialize(void* context, FlipperFormat* flipper_format);

void subghz_protocol_encoder_acurite_592txr_free(void* context);

LevelDuration subghz_protocol_encoder_acurite_592txr_yield(void* context);

void* subghz_protocol_encoder_acurite_592txr_alloc(SubGhzEnvironment* environment);