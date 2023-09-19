#include "wendox_w6726.h"

#define TAG "subghz_protocolWendoxW6726"

/*
 * Wendox W6726
 *   
 *  Temperature -50C to +70C
 *    _     _     _          __   _
 *  _| |___| |___| |___ ... |  |_| |__...._______________
 *    preamble                data           guard time
 * 
 *  3 reps every 3 minutes
 *  in the first message 11 bytes of the preamble in the rest by 7
 *  
 *  bit 0: 1955-hi, 5865-lo
 *  bit 1: 5865-hi, 1955-lo
 *  guard time: 12*1955+(lo last bit) 
 *  data: 29 bit
 * 
 *  IIIII | ZTTTTTTTTT | uuuuuuuBuu | CCCC
 * 
 *  I: identification;
 *  Z: temperature sign;
 *  T: temperature sign dependent +12C;
 *  B: battery low; flag to indicate low battery voltage;
 *  C: CRC4 (polynomial = 0x9, start_data = 0xD);
 *  u: unknown; 
 */

static const SubGhzBlockConst subghz_protocol_wendox_w6726_const = {
    .te_short = 1955,
    .te_long = 5865,
    .te_delta = 300,
    .min_count_bit_for_found = 29,
};

struct subghz_protocol_DecoderWendoxW6726 {
    SubGhzProtocolDecoderBase base;

    SubGhzBlockDecoder decoder;
    SubGhzBlockGeneric generic;

    uint16_t header_count;
};

struct subghz_protocol_EncoderWendoxW6726 {
    SubGhzProtocolEncoderBase base;

    SubGhzProtocolBlockEncoder encoder;
    SubGhzBlockGeneric generic;
};

typedef enum {
    WendoxW6726DecoderStepReset = 0,
    WendoxW6726DecoderStepCheckPreambule,
    WendoxW6726DecoderStepSaveDuration,
    WendoxW6726DecoderStepCheckDuration,
} WendoxW6726DecoderStep;

const SubGhzProtocolDecoder subghz_protocol_wendox_w6726_decoder = {
    .alloc = subghz_protocol_decoder_wendox_w6726_alloc,
    .free = subghz_protocol_decoder_wendox_w6726_free,

    .feed = subghz_protocol_decoder_wendox_w6726_feed,
    .reset = subghz_protocol_decoder_wendox_w6726_reset,

    .get_hash_data = subghz_protocol_decoder_wendox_w6726_get_hash_data,
    .serialize = subghz_protocol_decoder_wendox_w6726_serialize,
    .deserialize = subghz_protocol_decoder_wendox_w6726_deserialize,
    .get_string = subghz_protocol_decoder_wendox_w6726_get_string,
};

const SubGhzProtocolEncoder subghz_protocol_wendox_w6726_encoder = {
    .alloc = NULL,
    .free = NULL,

    .deserialize = NULL,
    .stop = NULL,
    .yield = NULL,
};

const SubGhzProtocol subghz_protocol_wendox_w6726 = {
    .name = subghz_protocol_WENDOX_W6726_NAME,
    //.type = SubGhzProtocolWeatherStation,
    .type = SubGhzProtocolTypeStatic,
    .flag = SubGhzProtocolFlag_433 | SubGhzProtocolFlag_315 | SubGhzProtocolFlag_868 |
            SubGhzProtocolFlag_AM | SubGhzProtocolFlag_Decodable,

    .decoder = &subghz_protocol_wendox_w6726_decoder,
    .encoder = &subghz_protocol_wendox_w6726_encoder,
};

void* subghz_protocol_decoder_wendox_w6726_alloc(SubGhzEnvironment* environment) {
    UNUSED(environment);
    subghz_protocol_DecoderWendoxW6726* instance = malloc(sizeof(subghz_protocol_DecoderWendoxW6726));
    instance->base.protocol = &subghz_protocol_wendox_w6726;
    instance->generic.protocol_name = instance->base.protocol->name;
    return instance;
}

void subghz_protocol_decoder_wendox_w6726_free(void* context) {
    furi_assert(context);
    subghz_protocol_DecoderWendoxW6726* instance = context;
    free(instance);
}

void subghz_protocol_decoder_wendox_w6726_reset(void* context) {
    furi_assert(context);
    subghz_protocol_DecoderWendoxW6726* instance = context;
    instance->decoder.parser_step = WendoxW6726DecoderStepReset;
}

static bool subghz_protocol_wendox_w6726_check(subghz_protocol_DecoderWendoxW6726* instance) {
    if(!instance->decoder.decode_data) return false;
    uint8_t msg[] = {
        instance->decoder.decode_data >> 28,
        instance->decoder.decode_data >> 20,
        instance->decoder.decode_data >> 12,
        instance->decoder.decode_data >> 4};

    uint8_t crc = subghz_protocol_blocks_crc4(msg, 4, 0x9, 0xD);
    return (crc == (instance->decoder.decode_data & 0x0F));
}

/**
 * Analysis of received data
 * @param instance Pointer to a SubGhzBlockGeneric* instance
 */
static void subghz_protocol_wendox_w6726_remote_controller(SubGhzBlockGeneric* instance) {
    instance->id = (instance->data >> 24) & 0xFF;
    instance->battery_low = (instance->data >> 6) & 1;
    instance->channel = WS_NO_CHANNEL;

    if(((instance->data >> 23) & 1)) {
        instance->temp = (float)(((instance->data >> 14) & 0x1FF) + 12) / 10.0f;
    } else {
        instance->temp = (float)((~(instance->data >> 14) & 0x1FF) + 1 - 12) / -10.0f;
    }

    if(instance->temp < -50.0f) {
        instance->temp = -50.0f;
    } else if(instance->temp > 70.0f) {
        instance->temp = 70.0f;
    }

    instance->btn = WS_NO_BTN;
    instance->humidity = WS_NO_HUMIDITY;
}

void subghz_protocol_decoder_wendox_w6726_feed(void* context, bool level, uint32_t duration) {
    furi_assert(context);
    subghz_protocol_DecoderWendoxW6726* instance = context;

    switch(instance->decoder.parser_step) {
    case WendoxW6726DecoderStepReset:
        if((level) && (DURATION_DIFF(duration, subghz_protocol_wendox_w6726_const.te_short) <
                       subghz_protocol_wendox_w6726_const.te_delta)) {
            instance->decoder.parser_step = WendoxW6726DecoderStepCheckPreambule;
            instance->decoder.te_last = duration;
            instance->header_count = 0;
        }
        break;

    case WendoxW6726DecoderStepCheckPreambule:
        if(level) {
            instance->decoder.te_last = duration;
        } else {
            if((DURATION_DIFF(instance->decoder.te_last, subghz_protocol_wendox_w6726_const.te_short) <
                subghz_protocol_wendox_w6726_const.te_delta * 1) &&
               (DURATION_DIFF(duration, subghz_protocol_wendox_w6726_const.te_long) <
                subghz_protocol_wendox_w6726_const.te_delta * 2)) {
                instance->header_count++;
            } else if((instance->header_count > 4) && (instance->header_count < 12)) {
                if((DURATION_DIFF(
                        instance->decoder.te_last, subghz_protocol_wendox_w6726_const.te_long) <
                    subghz_protocol_wendox_w6726_const.te_delta * 2) &&
                   (DURATION_DIFF(duration, subghz_protocol_wendox_w6726_const.te_short) <
                    subghz_protocol_wendox_w6726_const.te_delta)) {
                    instance->decoder.decode_data = 0;
                    instance->decoder.decode_count_bit = 0;
                    subghz_protocol_blocks_add_bit(&instance->decoder, 1);
                    instance->decoder.parser_step = WendoxW6726DecoderStepSaveDuration;
                } else {
                    instance->decoder.parser_step = WendoxW6726DecoderStepReset;
                }

            } else {
                instance->decoder.parser_step = WendoxW6726DecoderStepReset;
            }
        }
        break;

    case WendoxW6726DecoderStepSaveDuration:
        if(level) {
            instance->decoder.te_last = duration;
            instance->decoder.parser_step = WendoxW6726DecoderStepCheckDuration;
        } else {
            instance->decoder.parser_step = WendoxW6726DecoderStepReset;
        }
        break;

    case WendoxW6726DecoderStepCheckDuration:
        if(!level) {
            if(duration >
               subghz_protocol_wendox_w6726_const.te_short + subghz_protocol_wendox_w6726_const.te_long) {
                if(DURATION_DIFF(
                       instance->decoder.te_last, subghz_protocol_wendox_w6726_const.te_short) <
                   subghz_protocol_wendox_w6726_const.te_delta) {
                    subghz_protocol_blocks_add_bit(&instance->decoder, 0);
                    instance->decoder.parser_step = WendoxW6726DecoderStepSaveDuration;
                } else if(
                    DURATION_DIFF(
                        instance->decoder.te_last, subghz_protocol_wendox_w6726_const.te_long) <
                    subghz_protocol_wendox_w6726_const.te_delta * 2) {
                    subghz_protocol_blocks_add_bit(&instance->decoder, 1);
                    instance->decoder.parser_step = WendoxW6726DecoderStepSaveDuration;
                } else {
                    instance->decoder.parser_step = WendoxW6726DecoderStepReset;
                }
                if((instance->decoder.decode_count_bit ==
                    subghz_protocol_wendox_w6726_const.min_count_bit_for_found) &&
                   subghz_protocol_wendox_w6726_check(instance)) {
                    instance->generic.data = instance->decoder.decode_data;
                    instance->generic.data_count_bit = instance->decoder.decode_count_bit;
                    subghz_protocol_wendox_w6726_remote_controller(&instance->generic);
                    if(instance->base.callback)
                        instance->base.callback(&instance->base, instance->base.context);
                }

                instance->decoder.parser_step = WendoxW6726DecoderStepReset;
            } else if(
                (DURATION_DIFF(instance->decoder.te_last, subghz_protocol_wendox_w6726_const.te_short) <
                 subghz_protocol_wendox_w6726_const.te_delta) &&
                (DURATION_DIFF(duration, subghz_protocol_wendox_w6726_const.te_long) <
                 subghz_protocol_wendox_w6726_const.te_delta * 3)) {
                subghz_protocol_blocks_add_bit(&instance->decoder, 0);
                instance->decoder.parser_step = WendoxW6726DecoderStepSaveDuration;
            } else if(
                (DURATION_DIFF(instance->decoder.te_last, subghz_protocol_wendox_w6726_const.te_long) <
                 subghz_protocol_wendox_w6726_const.te_delta * 2) &&
                (DURATION_DIFF(duration, subghz_protocol_wendox_w6726_const.te_short) <
                 subghz_protocol_wendox_w6726_const.te_delta)) {
                subghz_protocol_blocks_add_bit(&instance->decoder, 1);
                instance->decoder.parser_step = WendoxW6726DecoderStepSaveDuration;
            } else {
                instance->decoder.parser_step = WendoxW6726DecoderStepReset;
            }
        } else {
            instance->decoder.parser_step = WendoxW6726DecoderStepReset;
        }
        break;
    }
}

uint8_t subghz_protocol_decoder_wendox_w6726_get_hash_data(void* context) {
    furi_assert(context);
    subghz_protocol_DecoderWendoxW6726* instance = context;
    return subghz_protocol_blocks_get_hash_data(
        &instance->decoder, (instance->decoder.decode_count_bit / 8) + 1);
}

SubGhzProtocolStatus subghz_protocol_decoder_wendox_w6726_serialize(
    void* context,
    FlipperFormat* flipper_format,
    SubGhzRadioPreset* preset) {
    furi_assert(context);
    subghz_protocol_DecoderWendoxW6726* instance = context;
    return subghz_block_generic_serialize(&instance->generic, flipper_format, preset);
}

SubGhzProtocolStatus
    subghz_protocol_decoder_wendox_w6726_deserialize(void* context, FlipperFormat* flipper_format) {
    furi_assert(context);
    subghz_protocol_DecoderWendoxW6726* instance = context;
    return subghz_block_generic_deserialize_check_count_bit(
        &instance->generic,
        flipper_format,
        subghz_protocol_wendox_w6726_const.min_count_bit_for_found);
}

void subghz_protocol_decoder_wendox_w6726_get_string(void* context, FuriString* output) {
    furi_assert(context);
    subghz_protocol_DecoderWendoxW6726* instance = context;
    furi_string_printf(
        output,
        "%s %dbit\r\n"
        "Key:0x%lX%08lX\r\n"
        "Sn:0x%lX Ch:%d  Bat:%d\r\n"
        "Temp:%3.1f C Hum:%d%%",
        instance->generic.protocol_name,
        instance->generic.data_count_bit,
        (uint32_t)(instance->generic.data >> 32),
        (uint32_t)(instance->generic.data),
        instance->generic.id,
        instance->generic.channel,
        instance->generic.battery_low,
        (double)instance->generic.temp,
        instance->generic.humidity);
}
