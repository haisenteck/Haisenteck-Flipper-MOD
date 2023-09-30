#include "oregon_v1.h"
#include <lib/toolbox/manchester_decoder.h>

#define TAG "subghz_protocol_Oregon_V1"

/*
 * Help
 * https://github.dev/merbanan/rtl_433/blob/bb1be7f186ac0fdb7dc5d77693847d96fb95281e/src/devices/oregon_scientific_v1.c
 * 
 * OSv1 protocol.
 * 
 * MC with nominal bit width of 2930 us.
 * Pulses are somewhat longer than nominal half-bit width, 1748 us / 3216 us,
 * Gaps are somewhat shorter than nominal half-bit width, 1176 us / 2640 us.
 * After 12 preamble bits there is 4200 us gap, 5780 us pulse, 5200 us gap.
 * And next 32 bit data
 * 
 * Care must be taken with the gap after the sync pulse since it
 * is outside of the normal clocking.  Because of this a data stream
 * beginning with a 0 will have data in this gap.   
 * 
 * 
 * Data is in reverse order of bits
 *      RevBit(data32bit)=> tib23atad
 * 
 *      tib23atad => xxxxxxxx | busuTTTT | ttttzzzz | ccuuiiii 
 *
 *      - i: ID
 *      - x: CRC;
 *      - u: unknown;
 *      - b: battery low; flag to indicate low battery voltage
 *      - s: temperature sign
 *      - T: BCD, Temperature; in °C * 10
 *      - t: BCD, Temperature; in °C * 1
 *      - z: BCD, Temperature; in °C * 0.1
 *      - c: Channel 00=CH1, 01=CH2, 10=CH3
 * 
 */

#define OREGON_V1_HEADER_OK 0xFF

static const SubGhzBlockConst subghz_protocol_oregon_v1_const = {
    .te_short = 1465,
    .te_long = 2930,
    .te_delta = 350,
    .min_count_bit_for_found = 32,
};

struct subghz_protocol_decoder_oregon_v1 {
    SubGhzProtocolDecoderBase base;

    SubGhzBlockDecoder decoder;
    SubGhzBlockGeneric generic;
    ManchesterState manchester_state;
    uint16_t header_count;
    uint8_t first_bit;
};

struct subghz_protocol_encoder_oregon_v1 {
    SubGhzProtocolEncoderBase base;

    SubGhzProtocolBlockEncoder encoder;
    SubGhzBlockGeneric generic;
};

typedef enum {
    Oregon_V1DecoderStepReset = 0,
    Oregon_V1DecoderStepFoundPreamble,
    Oregon_V1DecoderStepParse,
} Oregon_V1DecoderStep;

const SubGhzProtocolDecoder subghz_protocol_oregon_v1_decoder = {
    .alloc = subghz_protocol_decoder_oregon_v1_alloc,
    .free = subghz_protocol_decoder_oregon_v1_free,

    .feed = subghz_protocol_decoder_oregon_v1_feed,
    .reset = subghz_protocol_decoder_oregon_v1_reset,

    .get_hash_data = subghz_protocol_decoder_oregon_v1_get_hash_data,
    .serialize = subghz_protocol_decoder_oregon_v1_serialize,
    .deserialize = subghz_protocol_decoder_oregon_v1_deserialize,
    .get_string = subghz_protocol_decoder_oregon_v1_get_string,
};

const SubGhzProtocolEncoder subghz_protocol_oregon_v1_encoder = {
    .alloc = subghz_protocol_encoder_oregon_v1_alloc,
    .free = subghz_protocol_encoder_oregon_v1_free,

    .deserialize = subghz_protocol_encoder_oregon_v1_deserialize,
    .stop = subghz_protocol_encoder_oregon_v1_stop,
    .yield = subghz_protocol_encoder_oregon_v1_yield,
};

const SubGhzProtocol subghz_protocol_oregon_v1 = {
    .name = SUBGHZ_PROTOCOL_OREGON_V1_NAME,
    //.type = SubGhzProtocolWeatherStation,
    .type = SubGhzProtocolTypeStatic,
    .flag = SubGhzProtocolFlag_433 | SubGhzProtocolFlag_315 | SubGhzProtocolFlag_868 |
            SubGhzProtocolFlag_AM | SubGhzProtocolFlag_Decodable | SubGhzProtocolFlag_Load | SubGhzProtocolFlag_Save | SubGhzProtocolFlag_Send,

    .decoder = &subghz_protocol_oregon_v1_decoder,
    .encoder = &subghz_protocol_oregon_v1_encoder,
};

void* subghz_protocol_encoder_oregon_v1_alloc(SubGhzEnvironment* environment) {
    UNUSED(environment);
    subghz_protocol_encoder_oregon_v1* instance = malloc(sizeof(subghz_protocol_encoder_oregon_v1));

    instance->base.protocol = &subghz_protocol_oregon_v1;
    instance->generic.protocol_name = instance->base.protocol->name;

    instance->encoder.repeat = 10;
    instance->encoder.size_upload = 52;
    instance->encoder.upload = malloc(instance->encoder.size_upload * sizeof(LevelDuration));
    instance->encoder.is_running = false;
    return instance;
}

void* subghz_protocol_decoder_oregon_v1_alloc(SubGhzEnvironment* environment) {
    UNUSED(environment);
    subghz_protocol_decoder_oregon_v1* instance = malloc(sizeof(subghz_protocol_decoder_oregon_v1));
    instance->base.protocol = &subghz_protocol_oregon_v1;
    instance->generic.protocol_name = instance->base.protocol->name;
    return instance;
}

void subghz_protocol_decoder_oregon_v1_free(void* context) {
    furi_assert(context);
    subghz_protocol_decoder_oregon_v1* instance = context;
    free(instance);
}

void subghz_protocol_decoder_oregon_v1_reset(void* context) {
    furi_assert(context);
    subghz_protocol_decoder_oregon_v1* instance = context;
    instance->decoder.parser_step = Oregon_V1DecoderStepReset;
}

static bool subghz_protocol_oregon_v1_check(subghz_protocol_decoder_oregon_v1* instance) {
    if(!instance->decoder.decode_data) return false;
    uint64_t data = subghz_protocol_blocks_reverse_key(instance->decoder.decode_data, 32);
    uint16_t crc = (data & 0xff) + ((data >> 8) & 0xff) + ((data >> 16) & 0xff);
    crc = (crc & 0xff) + ((crc >> 8) & 0xff);
    return (crc == ((data >> 24) & 0xFF));
}

/**
 * Analysis of received data
 * @param instance Pointer to a SubGhzBlockGeneric* instance
 */
static void subghz_protocol_oregon_v1_remote_controller(SubGhzBlockGeneric* instance) {
    uint64_t data = subghz_protocol_blocks_reverse_key(instance->data, 32);

    instance->id = data & 0xFF;
    instance->channel = ((data >> 6) & 0x03) + 1;

    float temp_raw =
        ((data >> 8) & 0x0F) * 0.1f + ((data >> 12) & 0x0F) + ((data >> 16) & 0x0F) * 10.0f;
    if(!((data >> 21) & 1)) {
        instance->temp = temp_raw;
    } else {
        instance->temp = -temp_raw;
    }

    instance->battery_low = !((instance->data >> 23) & 1ULL);

    instance->btn = WS_NO_BTN;
    instance->humidity = WS_NO_HUMIDITY;
}

void subghz_protocol_decoder_oregon_v1_feed(void* context, bool level, uint32_t duration) {
    furi_assert(context);
    subghz_protocol_decoder_oregon_v1* instance = context;
    ManchesterEvent event = ManchesterEventReset;
    switch(instance->decoder.parser_step) {
    case Oregon_V1DecoderStepReset:
        if((level) && (DURATION_DIFF(duration, subghz_protocol_oregon_v1_const.te_short) <
                       subghz_protocol_oregon_v1_const.te_delta)) {
            instance->decoder.parser_step = Oregon_V1DecoderStepFoundPreamble;
            instance->decoder.te_last = duration;
            instance->header_count = 0;
        }
        break;
    case Oregon_V1DecoderStepFoundPreamble:
        if(level) {
            //keep high levels, if they suit our durations
            if((DURATION_DIFF(duration, subghz_protocol_oregon_v1_const.te_short) <
                subghz_protocol_oregon_v1_const.te_delta) ||
               (DURATION_DIFF(duration, subghz_protocol_oregon_v1_const.te_short * 4) <
                subghz_protocol_oregon_v1_const.te_delta)) {
                instance->decoder.te_last = duration;
            } else {
                instance->decoder.parser_step = Oregon_V1DecoderStepReset;
            }
        } else if(
            //checking low levels
            (DURATION_DIFF(duration, subghz_protocol_oregon_v1_const.te_short) <
             subghz_protocol_oregon_v1_const.te_delta) &&
            (DURATION_DIFF(instance->decoder.te_last, subghz_protocol_oregon_v1_const.te_short) <
             subghz_protocol_oregon_v1_const.te_delta)) {
            // Found header
            instance->header_count++;
        } else if(
            (DURATION_DIFF(duration, subghz_protocol_oregon_v1_const.te_short * 3) <
             subghz_protocol_oregon_v1_const.te_delta) &&
            (DURATION_DIFF(instance->decoder.te_last, subghz_protocol_oregon_v1_const.te_short) <
             subghz_protocol_oregon_v1_const.te_delta)) {
            // check header
            if(instance->header_count > 7) {
                instance->header_count = OREGON_V1_HEADER_OK;
            }
        } else if(
            (instance->header_count == OREGON_V1_HEADER_OK) &&
            (DURATION_DIFF(instance->decoder.te_last, subghz_protocol_oregon_v1_const.te_short * 4) <
             subghz_protocol_oregon_v1_const.te_delta)) {
            //found all the necessary patterns
            instance->decoder.decode_data = 0;
            instance->decoder.decode_count_bit = 1;
            manchester_advance(
                instance->manchester_state,
                ManchesterEventReset,
                &instance->manchester_state,
                NULL);
            instance->decoder.parser_step = Oregon_V1DecoderStepParse;
            if(duration < subghz_protocol_oregon_v1_const.te_short * 4) {
                instance->first_bit = 1;
            } else {
                instance->first_bit = 0;
            }
        } else {
            instance->decoder.parser_step = Oregon_V1DecoderStepReset;
        }
        break;
    case Oregon_V1DecoderStepParse:
        if(level) {
            if(DURATION_DIFF(duration, subghz_protocol_oregon_v1_const.te_short) <
               subghz_protocol_oregon_v1_const.te_delta) {
                event = ManchesterEventShortHigh;
            } else if(
                DURATION_DIFF(duration, subghz_protocol_oregon_v1_const.te_long) <
                subghz_protocol_oregon_v1_const.te_delta) {
                event = ManchesterEventLongHigh;
            } else {
                instance->decoder.parser_step = Oregon_V1DecoderStepReset;
            }
        } else {
            if(DURATION_DIFF(duration, subghz_protocol_oregon_v1_const.te_short) <
               subghz_protocol_oregon_v1_const.te_delta) {
                event = ManchesterEventShortLow;
            } else if(
                DURATION_DIFF(duration, subghz_protocol_oregon_v1_const.te_long) <
                subghz_protocol_oregon_v1_const.te_delta) {
                event = ManchesterEventLongLow;
            } else if(duration >= ((uint32_t)subghz_protocol_oregon_v1_const.te_long * 2)) {
                if(instance->decoder.decode_count_bit ==
                   subghz_protocol_oregon_v1_const.min_count_bit_for_found) {
                    if(instance->first_bit) {
                        instance->decoder.decode_data = ~instance->decoder.decode_data | (1 << 31);
                    }
                    if(subghz_protocol_oregon_v1_check(instance)) {
                        instance->generic.data = instance->decoder.decode_data;
                        instance->generic.data_count_bit = instance->decoder.decode_count_bit;
                        subghz_protocol_oregon_v1_remote_controller(&instance->generic);
                        if(instance->base.callback)
                            instance->base.callback(&instance->base, instance->base.context);
                    }
                }
                instance->decoder.decode_data = 0;
                instance->decoder.decode_count_bit = 0;
                manchester_advance(
                    instance->manchester_state,
                    ManchesterEventReset,
                    &instance->manchester_state,
                    NULL);
            } else {
                instance->decoder.parser_step = Oregon_V1DecoderStepReset;
            }
        }
        if(event != ManchesterEventReset) {
            bool data;
            bool data_ok = manchester_advance(
                instance->manchester_state, event, &instance->manchester_state, &data);

            if(data_ok) {
                instance->decoder.decode_data = (instance->decoder.decode_data << 1) | !data;
                instance->decoder.decode_count_bit++;
            }
        }

        break;
    }
}

void subghz_protocol_encoder_oregon_v1_free(void* context) {
    furi_assert(context);
    subghz_protocol_encoder_oregon_v1* instance = context;
    free(instance->encoder.upload);
    free(instance);
}

uint8_t subghz_protocol_decoder_oregon_v1_get_hash_data(void* context) {
    furi_assert(context);
    subghz_protocol_decoder_oregon_v1* instance = context;
    return subghz_protocol_blocks_get_hash_data(
        &instance->decoder, (instance->decoder.decode_count_bit / 8) + 1);
}

SubGhzProtocolStatus subghz_protocol_decoder_oregon_v1_serialize(
    void* context,
    FlipperFormat* flipper_format,
    SubGhzRadioPreset* preset) {
    furi_assert(context);
    subghz_protocol_decoder_oregon_v1* instance = context;
    return subghz_block_generic_serialize(&instance->generic, flipper_format, preset);
}

static bool subghz_protocol_encoder_oregon_v1_get_upload(subghz_protocol_encoder_oregon_v1* instance) {
    furi_assert(instance);
    size_t index = 0;
    size_t size_upload = (instance->generic.data_count_bit * 2);
    if(size_upload > instance->encoder.size_upload) {
        FURI_LOG_E(TAG, "Size upload exceeds allocated encoder buffer.");
        return false;
    } else {
        instance->encoder.size_upload = size_upload;
    }

    for(uint8_t i = instance->generic.data_count_bit; i > 1; i--) {
        if(bit_read(instance->generic.data, i - 1)) {
            //send bit 1
            instance->encoder.upload[index++] =
                level_duration_make(true, (uint32_t)subghz_protocol_oregon_v1_const.te_long);
            instance->encoder.upload[index++] =
                level_duration_make(false, (uint32_t)subghz_protocol_oregon_v1_const.te_short);
        } else {
            //send bit 0
            instance->encoder.upload[index++] =
                level_duration_make(true, (uint32_t)subghz_protocol_oregon_v1_const.te_short);
            instance->encoder.upload[index++] =
                level_duration_make(false, (uint32_t)subghz_protocol_oregon_v1_const.te_long);
        }
    }
    if(bit_read(instance->generic.data, 0)) {
        //send bit 1
        instance->encoder.upload[index++] =
            level_duration_make(true, (uint32_t)subghz_protocol_oregon_v1_const.te_long);
        instance->encoder.upload[index++] = level_duration_make(
            false,
            (uint32_t)subghz_protocol_oregon_v1_const.te_short +
                subghz_protocol_oregon_v1_const.te_long * 7);
    } else {
        //send bit 0
        instance->encoder.upload[index++] =
            level_duration_make(true, (uint32_t)subghz_protocol_oregon_v1_const.te_short);
        instance->encoder.upload[index++] = level_duration_make(
            false,
            (uint32_t)subghz_protocol_oregon_v1_const.te_long +
                subghz_protocol_oregon_v1_const.te_long * 7);
    }
    return true;
}

LevelDuration subghz_protocol_encoder_oregon_v1_yield(void* context) {
    subghz_protocol_encoder_oregon_v1* instance = context;

    if(instance->encoder.repeat == 0 || !instance->encoder.is_running) {
        instance->encoder.is_running = false;
        return level_duration_reset();
    }

    LevelDuration ret = instance->encoder.upload[instance->encoder.front];

    if(++instance->encoder.front == instance->encoder.size_upload) {
        instance->encoder.repeat--;
        instance->encoder.front = 0;
    }

    return ret;
}

void subghz_protocol_encoder_oregon_v1_stop(void* context) {
    subghz_protocol_encoder_oregon_v1* instance = context;
    instance->encoder.is_running = false;
}

SubGhzProtocolStatus subghz_protocol_decoder_oregon_v1_deserialize(void* context, FlipperFormat* flipper_format) {
    furi_assert(context);
    subghz_protocol_decoder_oregon_v1* instance = context;
    return subghz_block_generic_deserialize_check_count_bit(
        &instance->generic, flipper_format, subghz_protocol_oregon_v1_const.min_count_bit_for_found);
}

SubGhzProtocolStatus subghz_protocol_encoder_oregon_v1_deserialize(void* context, FlipperFormat* flipper_format) {
    furi_assert(context);
    subghz_protocol_encoder_oregon_v1* instance = context;
    SubGhzProtocolStatus ret = SubGhzProtocolStatusError;
    do {
        ret = subghz_block_generic_deserialize_check_count_bit(
            &instance->generic,
            flipper_format,
            subghz_protocol_oregon_v1_const.min_count_bit_for_found);
        if(ret != SubGhzProtocolStatusOk) {
            break;
        }
        //optional parameter parameter
        flipper_format_read_uint32(
            flipper_format, "Repeat", (uint32_t*)&instance->encoder.repeat, 1);

        if(!subghz_protocol_encoder_oregon_v1_get_upload(instance)) {
            ret = SubGhzProtocolStatusErrorEncoderGetUpload;
            break;
        }
        instance->encoder.is_running = true;

    } while(false);

    return ret;
}

void subghz_protocol_decoder_oregon_v1_get_string(void* context, FuriString* output) {
    furi_assert(context);
    subghz_protocol_decoder_oregon_v1* instance = context;
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
