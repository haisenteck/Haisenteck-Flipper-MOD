#include "oregon2.h"
#include <lib/toolbox/manchester_decoder.h>

#define TAG "SubGhzProtocolOregon2"

static const SubGhzBlockConst subghz_protocol_oregon2_const = {
    .te_long = 1000,
    .te_short = 500,
    .te_delta = 200,
    .min_count_bit_for_found = 32,
};

#define OREGON2_PREAMBLE_BITS 19
#define OREGON2_PREAMBLE_MASK ((1 << (OREGON2_PREAMBLE_BITS + 1)) - 1)
#define OREGON2_SENSOR_ID(d) (((d) >> 16) & 0xFFFF)
#define OREGON2_CHECKSUM_BITS 8

// 15 ones + 0101 (inverted A)
#define OREGON2_PREAMBLE 0b1111111111111110101

// bit indicating the low battery
#define OREGON2_FLAG_BAT_LOW 0x4

struct SubGhzProtocolDecoderOregon2 {
    SubGhzProtocolDecoderBase base;

    SubGhzBlockDecoder decoder;
    SubGhzBlockGeneric generic;
    ManchesterState manchester_state;
    bool prev_bit;
    bool have_bit;

    uint8_t var_bits;
    uint32_t var_data;
};

typedef struct SubGhzProtocolDecoderOregon2 SubGhzProtocolDecoderOregon2;

typedef enum {
    Oregon2DecoderStepReset = 0,
    Oregon2DecoderStepFoundPreamble,
    Oregon2DecoderStepVarData,
} Oregon2DecoderStep;

void* subghz_protocol_decoder_oregon2_alloc(SubGhzEnvironment* environment) {
    UNUSED(environment);
    SubGhzProtocolDecoderOregon2* instance = malloc(sizeof(SubGhzProtocolDecoderOregon2));
    instance->base.protocol = &subghz_protocol_oregon2;
    instance->generic.protocol_name = instance->base.protocol->name;
    return instance;
}

void subghz_protocol_decoder_oregon2_free(void* context) {
    furi_assert(context);
    SubGhzProtocolDecoderOregon2* instance = context;
    free(instance);
}

void subghz_protocol_decoder_oregon2_reset(void* context) {
    furi_assert(context);
    SubGhzProtocolDecoderOregon2* instance = context;
    instance->decoder.parser_step = Oregon2DecoderStepReset;
    instance->decoder.decode_data = 0UL;
    instance->decoder.decode_count_bit = 0;
    manchester_advance(
        instance->manchester_state, ManchesterEventReset, &instance->manchester_state, NULL);
    instance->have_bit = false;
    instance->var_data = 0;
    instance->var_bits = 0;
}

static ManchesterEvent level_and_duration_to_event(bool level, uint32_t duration) {
    bool is_long = false;

    if(DURATION_DIFF(duration, subghz_protocol_oregon2_const.te_long) < subghz_protocol_oregon2_const.te_delta) {
        is_long = true;
    } else if(DURATION_DIFF(duration, subghz_protocol_oregon2_const.te_short) < subghz_protocol_oregon2_const.te_delta) {
        is_long = false;
    } else {
        return ManchesterEventReset;
    }

    if(level)
        return is_long ? ManchesterEventLongHigh : ManchesterEventShortHigh;
    else
        return is_long ? ManchesterEventLongLow : ManchesterEventShortLow;
}

// From sensor id code return amount of bits in variable section
static uint8_t oregon2_sensor_id_var_bits(uint16_t sensor_id) {
    if(sensor_id == 0xEC40) return 16;
    return 0;
}

void subghz_protocol_decoder_oregon2_feed(void* context, bool level, uint32_t duration) {
    furi_assert(context);
    SubGhzProtocolDecoderOregon2* instance = context;
    // oregon v2.1 signal is inverted
    ManchesterEvent event = level_and_duration_to_event(!level, duration);
    bool data;

    // low-level bit sequence decoding
    if(event == ManchesterEventReset) {
        instance->decoder.parser_step = Oregon2DecoderStepReset;
        instance->have_bit = false;
        instance->decoder.decode_data = 0UL;
        instance->decoder.decode_count_bit = 0;
    }
    if(manchester_advance(instance->manchester_state, event, &instance->manchester_state, &data)) {
        if(instance->have_bit) {
            if(!instance->prev_bit && data) {
                subghz_protocol_blocks_add_bit(&instance->decoder, 1);
            } else if(instance->prev_bit && !data) {
                subghz_protocol_blocks_add_bit(&instance->decoder, 0);
            } else {
                subghz_protocol_decoder_oregon2_reset(context);
            }
            instance->have_bit = false;
        } else {
            instance->prev_bit = data;
            instance->have_bit = true;
        }
    }

    switch(instance->decoder.parser_step) {
    case Oregon2DecoderStepReset:
        // waiting for fixed oregon2 preamble
        if(instance->decoder.decode_count_bit >= OREGON2_PREAMBLE_BITS &&
           ((instance->decoder.decode_data & OREGON2_PREAMBLE_MASK) == OREGON2_PREAMBLE)) {
            instance->decoder.parser_step = Oregon2DecoderStepFoundPreamble;
            instance->decoder.decode_count_bit = 0;
            instance->decoder.decode_data = 0UL;
        }
        break;
    case Oregon2DecoderStepFoundPreamble:
        // waiting for fixed oregon2 data
        if(instance->decoder.decode_count_bit == 32) {
            instance->generic.data = instance->decoder.decode_data;
            instance->generic.data_count_bit = instance->decoder.decode_count_bit;
            instance->decoder.decode_data = 0UL;
            instance->decoder.decode_count_bit = 0;

            // reverse nibbles in decoded data
            instance->generic.data = (instance->generic.data & 0x55555555) << 1 |
                                     (instance->generic.data & 0xAAAAAAAA) >> 1;
            instance->generic.data = (instance->generic.data & 0x33333333) << 2 |
                                     (instance->generic.data & 0xCCCCCCCC) >> 2;

            instance->var_bits =
                oregon2_sensor_id_var_bits(OREGON2_SENSOR_ID(instance->generic.data));

            if(!instance->var_bits) {
                // sensor is not supported, stop decoding, but showing the decoded fixed part
                instance->decoder.parser_step = Oregon2DecoderStepReset;
                if(instance->base.callback)
                    instance->base.callback(&instance->base, instance->base.context);
            } else {
                instance->decoder.parser_step = Oregon2DecoderStepVarData;
            }
        }
        break;
    case Oregon2DecoderStepVarData:
        // waiting for variable (sensor-specific data)
        if(instance->decoder.decode_count_bit == instance->var_bits + OREGON2_CHECKSUM_BITS) {
            instance->var_data = instance->decoder.decode_data & 0xFFFFFFFF;

            // reverse nibbles in var data
            instance->var_data = (instance->var_data & 0x55555555) << 1 |
                                 (instance->var_data & 0xAAAAAAAA) >> 1;
            instance->var_data = (instance->var_data & 0x33333333) << 2 |
                                 (instance->var_data & 0xCCCCCCCC) >> 2;

            instance->decoder.parser_step = Oregon2DecoderStepReset;
            if(instance->base.callback)
                instance->base.callback(&instance->base, instance->base.context);
        }
        break;
    }
}

uint8_t subghz_protocol_decoder_oregon2_get_hash_data(void* context) {
    furi_assert(context);
    SubGhzProtocolDecoderOregon2* instance = context;
    return subghz_protocol_blocks_get_hash_data(
        &instance->decoder, (instance->decoder.decode_count_bit / 8) + 1);
}

SubGhzProtocolStatus subghz_protocol_decoder_oregon2_serialize(
    void* context,
    FlipperFormat* flipper_format,
    SubGhzRadioPreset* preset) {
    furi_assert(context);
    SubGhzProtocolDecoderOregon2* instance = context;
    if(!subghz_block_generic_serialize(&instance->generic, flipper_format, preset)) return false;
    uint32_t temp = instance->var_bits;
    if(!flipper_format_write_uint32(flipper_format, "VarBits", &temp, 1)) {
        FURI_LOG_E(TAG, "Error adding VarBits");
        return false;
    }
    if(!flipper_format_write_hex(
           flipper_format,
           "VarData",
           (const uint8_t*)&instance->var_data,
           sizeof(instance->var_data))) {
        FURI_LOG_E(TAG, "Error adding VarData");
        return false;
    }
    return true;
}

SubGhzProtocolStatus subghz_protocol_decoder_oregon2_deserialize(void* context, FlipperFormat* flipper_format) {
    furi_assert(context);
    SubGhzProtocolDecoderOregon2* instance = context;
    bool ret = false;
    uint32_t temp_data;
    do {
        if(!subghz_block_generic_deserialize(&instance->generic, flipper_format)) {
            break;
        }
        if(!flipper_format_read_uint32(flipper_format, "VarBits", &temp_data, 1)) {
            FURI_LOG_E(TAG, "Missing VarLen");
            break;
        }
        instance->var_bits = (uint8_t)temp_data;
        if(!flipper_format_read_hex(
               flipper_format,
               "VarData",
               (uint8_t*)&instance->var_data,
               sizeof(instance->var_data))) {
            FURI_LOG_E(TAG, "Missing VarData");
            break;
        }
        if(instance->generic.data_count_bit != subghz_protocol_oregon2_const.min_count_bit_for_found) {
            FURI_LOG_E(TAG, "Wrong number of bits in key: %d", instance->generic.data_count_bit);
            break;
        }
        ret = true;
    } while(false);
    return ret;
}

// append string of the variable data
static void oregon2_var_data_append_string(uint16_t sensor_id, uint32_t var_data, FuriString* output) {
    uint32_t val;

    if(sensor_id == 0xEC40) {
        val = ((var_data >> 4) & 0xF) * 10 + ((var_data >> 8) & 0xF);
        furi_string_cat_printf(
            output,
            "Temp: %s%ld.%ld C\r\n",
            (var_data & 0xF) ? "-" : "+",
            val,
            (uint32_t)(var_data >> 12) & 0xF);
    }
}

static void oregon2_append_check_sum(uint32_t fix_data, uint32_t var_data, FuriString* output) {
    uint8_t sum = fix_data & 0xF;
    uint8_t ref_sum = var_data & 0xFF;
    var_data >>= 8;

    for(uint8_t i = 1; i < 8; i++) {
        fix_data >>= 4;
        var_data >>= 4;
        sum += (fix_data & 0xF) + (var_data & 0xF);
    }

    // swap calculated sum nibbles
    sum = (((sum >> 4) & 0xF) | (sum << 4)) & 0xFF;
    if(sum == ref_sum)
        furi_string_cat_printf(output, "Sum ok: 0x%hhX", ref_sum);
    else
        furi_string_cat_printf(output, "Sum err: 0x%hhX vs 0x%hhX", ref_sum, sum);
}

void subghz_protocol_decoder_oregon2_get_string(void* context, FuriString* output) {
    furi_assert(context);
    SubGhzProtocolDecoderOregon2* instance = context;
    uint16_t sensor_id = OREGON2_SENSOR_ID(instance->generic.data);
    furi_string_cat_printf(
        output,
        "%s\r\n"
        "ID: 0x%04lX, ch: %ld%s, rc: 0x%02lX\r\n",
        instance->generic.protocol_name,
        (uint32_t)sensor_id,
        (uint32_t)(instance->generic.data >> 12) & 0xF,
        ((instance->generic.data & OREGON2_FLAG_BAT_LOW) ? ", low bat" : ""),
        (uint32_t)(instance->generic.data >> 4) & 0xFF);

    if(instance->var_bits > 0) {
        oregon2_var_data_append_string(
            sensor_id, instance->var_data >> OREGON2_CHECKSUM_BITS, output);
        oregon2_append_check_sum((uint32_t)instance->generic.data, instance->var_data, output);
    }
}

const SubGhzProtocolDecoder subghz_protocol_oregon2_decoder = {
    .alloc = subghz_protocol_decoder_oregon2_alloc,
    .free = subghz_protocol_decoder_oregon2_free,

    .feed = subghz_protocol_decoder_oregon2_feed,
    .reset = subghz_protocol_decoder_oregon2_reset,

    .get_hash_data = subghz_protocol_decoder_oregon2_get_hash_data,
    .serialize = subghz_protocol_decoder_oregon2_serialize,
    .deserialize = subghz_protocol_decoder_oregon2_deserialize,
    .get_string = subghz_protocol_decoder_oregon2_get_string,
};

struct subghz_protocol_encoder_oregon2 {
    SubGhzProtocolEncoderBase base;

    SubGhzProtocolBlockEncoder encoder;
    SubGhzBlockGeneric generic;
};

const SubGhzProtocol subghz_protocol_oregon2 = {
    .name = SUBGHZ_PROTOCOL_OREGON2_NAME,
    .type = SubGhzProtocolTypeStatic,
    .flag = SubGhzProtocolFlag_433 | SubGhzProtocolFlag_AM | SubGhzProtocolFlag_Decodable | SubGhzProtocolFlag_Load | SubGhzProtocolFlag_Save | SubGhzProtocolFlag_Send,

    .decoder = &subghz_protocol_oregon2_decoder,
    .encoder = &subghz_protocol_oregon2_encoder,
};



const SubGhzProtocolEncoder subghz_protocol_oregon2_encoder = {
    .alloc = subghz_protocol_encoder_oregon2_alloc,
    .free = subghz_protocol_encoder_oregon2_free,

    .deserialize = subghz_protocol_encoder_oregon2_deserialize,
    .stop = subghz_protocol_encoder_oregon2_stop,
    .yield = subghz_protocol_encoder_oregon2_yield,
};

void* subghz_protocol_encoder_oregon2_alloc(SubGhzEnvironment* environment) {
    UNUSED(environment);
    subghz_protocol_encoder_oregon2* instance = malloc(sizeof(subghz_protocol_encoder_oregon2));

    instance->base.protocol = &subghz_protocol_oregon2;
    instance->generic.protocol_name = instance->base.protocol->name;

    instance->encoder.repeat = 10;
    instance->encoder.size_upload = 52;
    instance->encoder.upload = malloc(instance->encoder.size_upload * sizeof(LevelDuration));
    instance->encoder.is_running = false;
    return instance;
}

void subghz_protocol_encoder_oregon2_free(void* context) {
    furi_assert(context);
    subghz_protocol_encoder_oregon2* instance = context;
    free(instance->encoder.upload);
    free(instance);
}

static bool subghz_protocol_encoder_oregon2_get_upload(subghz_protocol_encoder_oregon2* instance) {
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
                level_duration_make(true, (uint32_t)subghz_protocol_oregon2_const.te_long);
            instance->encoder.upload[index++] =
                level_duration_make(false, (uint32_t)subghz_protocol_oregon2_const.te_short);
        } else {
            //send bit 0
            instance->encoder.upload[index++] =
                level_duration_make(true, (uint32_t)subghz_protocol_oregon2_const.te_short);
            instance->encoder.upload[index++] =
                level_duration_make(false, (uint32_t)subghz_protocol_oregon2_const.te_long);
        }
    }
    if(bit_read(instance->generic.data, 0)) {
        //send bit 1
        instance->encoder.upload[index++] =
            level_duration_make(true, (uint32_t)subghz_protocol_oregon2_const.te_long);
        instance->encoder.upload[index++] = level_duration_make(
            false,
            (uint32_t)subghz_protocol_oregon2_const.te_short +
                subghz_protocol_oregon2_const.te_long * 7);
    } else {
        //send bit 0
        instance->encoder.upload[index++] =
            level_duration_make(true, (uint32_t)subghz_protocol_oregon2_const.te_short);
        instance->encoder.upload[index++] = level_duration_make(
            false,
            (uint32_t)subghz_protocol_oregon2_const.te_long +
                subghz_protocol_oregon2_const.te_long * 7);
    }
    return true;
}

LevelDuration subghz_protocol_encoder_oregon2_yield(void* context) {
    subghz_protocol_encoder_oregon2* instance = context;

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

void subghz_protocol_encoder_oregon2_stop(void* context) {
    subghz_protocol_encoder_oregon2* instance = context;
    instance->encoder.is_running = false;
}

SubGhzProtocolStatus subghz_protocol_encoder_oregon2_deserialize(void* context, FlipperFormat* flipper_format) {
    furi_assert(context);
    subghz_protocol_encoder_oregon2* instance = context;
    SubGhzProtocolStatus ret = SubGhzProtocolStatusError;
    do {
        ret = subghz_block_generic_deserialize_check_count_bit(
            &instance->generic,
            flipper_format,
            subghz_protocol_oregon2_const.min_count_bit_for_found);
        if(ret != SubGhzProtocolStatusOk) {
            break;
        }
        //optional parameter parameter
        flipper_format_read_uint32(
            flipper_format, "Repeat", (uint32_t*)&instance->encoder.repeat, 1);

        if(!subghz_protocol_encoder_oregon2_get_upload(instance)) {
            ret = SubGhzProtocolStatusErrorEncoderGetUpload;
            break;
        }
        instance->encoder.is_running = true;

    } while(false);

    return ret;
}