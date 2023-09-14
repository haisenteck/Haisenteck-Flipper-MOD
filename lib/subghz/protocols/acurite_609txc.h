#pragma once

#include "base.h"

#include "../blocks/const.h"
#include "../blocks/decoder.h"
#include "../blocks/encoder.h"
#include "../blocks/generic.h"
#include "../blocks/math.h"

#define subghz_protocol_ACURITE_609TXC_NAME "Acurite-609TXC"

typedef struct subghz_protocolDecoderAcurite_609TXC subghz_protocolDecoderAcurite_609TXC;
typedef struct subghz_protocolEncoderAcurite_609TXC subghz_protocolEncoderAcurite_609TXC;

extern const SubGhzProtocolDecoder subghz_protocol_acurite_609txc_decoder;
extern const SubGhzProtocolEncoder subghz_protocol_acurite_609txc_encoder;
extern const SubGhzProtocol subghz_protocol_acurite_609txc;

/**
 * Allocate subghz_protocolDecoderAcurite_609TXC.
 * @param environment Pointer to a SubGhzEnvironment instance
 * @return subghz_protocolDecoderAcurite_609TXC* pointer to a subghz_protocolDecoderAcurite_609TXC instance
 */
void* subghz_protocol_decoder_acurite_609txc_alloc(SubGhzEnvironment* environment);

/**
 * Free subghz_protocolDecoderAcurite_609TXC.
 * @param context Pointer to a subghz_protocolDecoderAcurite_609TXC instance
 */
void subghz_protocol_decoder_acurite_609txc_free(void* context);

/**
 * Reset decoder subghz_protocolDecoderAcurite_609TXC.
 * @param context Pointer to a subghz_protocolDecoderAcurite_609TXC instance
 */
void subghz_protocol_decoder_acurite_609txc_reset(void* context);

/**
 * Parse a raw sequence of levels and durations received from the air.
 * @param context Pointer to a subghz_protocolDecoderAcurite_609TXC instance
 * @param level Signal level true-high false-low
 * @param duration Duration of this level in, us
 */
void subghz_protocol_decoder_acurite_609txc_feed(void* context, bool level, uint32_t duration);

/**
 * Getting the hash sum of the last randomly received parcel.
 * @param context Pointer to a subghz_protocolDecoderAcurite_609TXC instance
 * @return hash Hash sum
 */
uint8_t subghz_protocol_decoder_acurite_609txc_get_hash_data(void* context);

/**
 * Serialize data subghz_protocolDecoderAcurite_609TXC.
 * @param context Pointer to a subghz_protocolDecoderAcurite_609TXC instance
 * @param flipper_format Pointer to a FlipperFormat instance
 * @param preset The modulation on which the signal was received, SubGhzRadioPreset
 * @return status
 */
SubGhzProtocolStatus subghz_protocol_decoder_acurite_609txc_serialize(
    void* context,
    FlipperFormat* flipper_format,
    SubGhzRadioPreset* preset);

/**
 * Deserialize data subghz_protocolDecoderAcurite_609TXC.
 * @param context Pointer to a subghz_protocolDecoderAcurite_609TXC instance
 * @param flipper_format Pointer to a FlipperFormat instance
 * @return status
 */
SubGhzProtocolStatus
    subghz_protocol_decoder_acurite_609txc_deserialize(void* context, FlipperFormat* flipper_format);

/**
 * Getting a textual representation of the received data.
 * @param context Pointer to a subghz_protocolDecoderAcurite_609TXC instance
 * @param output Resulting text
 */
void subghz_protocol_decoder_acurite_609txc_get_string(void* context, FuriString* output);
