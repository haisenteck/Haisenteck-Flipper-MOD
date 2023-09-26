#include "baldr_rain.h"
#include "../blocks/const.h"
#include "../blocks/decoder.h"
#include "../blocks/encoder.h"
#include "../blocks/generic.h"
#include "../blocks/math.h"

#define TAG "SubGhzProtocol_baldr_rain"

//#define DIP_PATTERN "%c%c%c%c%c%c%c%c"


/** @fn int baldr_rain_decode(r_device *decoder, bitbuffer_t *bitbuffer)
Baldr / RainPoint Rain Gauge protocol.

For Baldr Wireless Weather Station with Rain Gauge.
See #2394

Only reports rain. There's a separate temperature sensor captured by Nexus-TH.

The sensor sends 36 bits 13 times,
the packets are ppm modulated (distance coding) with a pulse of ~500 us
followed by a short gap of ~1000 us for a 0 bit or a long ~2000 us gap for a
1 bit, the sync gap is ~4000 us.

Sample data:

    {36}75b000000 [0 mm]
    {36}75b000027 [0.9 mm]
    {36}75b000050 [2.0 mm]
    {36}75b8000cf [5.2 mm]
    {36}75b80017a [9.6 mm]
    {36}75b800224 [13.9 mm]
    {36}75b8002a3 [17.1 mm]

The data is grouped in 9 nibbles:

    II IF RR RR R

- I : 8 or 12-bit ID, could contain a model type nibble
- F : 4 bit, some flags
- R : 20 bit rain in inch/1000

*/
int rubicson_crc_check(uint8_t *b);

static const SubGhzBlockConst subghz_protocol_baldr_rain_const = {
    .te_short = 1000,
    .te_long = 2000,
    .te_delta = 3000,
    .min_count_bit_for_found = 36,
};

struct SubGhzProtocolDecoder_baldr_rain {
    SubGhzProtocolDecoderBase base;

    SubGhzBlockDecoder decoder;
    SubGhzBlockGeneric generic;
};

struct SubGhzProtocolEncoder_baldr_rain {
    SubGhzProtocolEncoderBase base;

    SubGhzProtocolBlockEncoder encoder;
    SubGhzBlockGeneric generic;
};

typedef enum {
    _baldr_rainDecoderStepReset = 0,
    _baldr_rainDecoderStepFoundStartBit,
    _baldr_rainDecoderStepSaveDuration,
    _baldr_rainDecoderStepCheckDuration,
} _baldr_rainDecoderStep;

const SubGhzProtocolDecoder subghz_protocol_baldr_rain_decoder = {
    .alloc = subghz_protocol_decoder_baldr_rain_alloc,
    .free = subghz_protocol_decoder_baldr_rain_free,

    .feed = subghz_protocol_decoder_baldr_rain_feed,
    .reset = subghz_protocol_decoder_baldr_rain_reset,

    .get_hash_data = subghz_protocol_decoder_baldr_rain_get_hash_data,
    .serialize = subghz_protocol_decoder_baldr_rain_serialize,
    .deserialize = subghz_protocol_decoder_baldr_rain_deserialize,
    .get_string = subghz_protocol_decoder_baldr_rain_get_string,
};

const SubGhzProtocolEncoder subghz_protocol_baldr_rain_encoder = {
    .alloc = subghz_protocol_encoder_baldr_rain_alloc,
    .free = subghz_protocol_encoder_baldr_rain_free,

    .deserialize = subghz_protocol_encoder_baldr_rain_deserialize,
    .stop = subghz_protocol_encoder_baldr_rain_stop,
    .yield = subghz_protocol_encoder_baldr_rain_yield,
};

const SubGhzProtocol subghz_protocol_baldr_rain = {
    .name = SUBGHZ_PROTOCOL_BALDR_RAIN_NAME,
    .type = SubGhzProtocolTypeStatic,
    .flag = SubGhzProtocolFlag_433 | SubGhzProtocolFlag_FM | SubGhzProtocolFlag_Decodable | SubGhzProtocolFlag_Load | SubGhzProtocolFlag_Save | SubGhzProtocolFlag_Send,

    .decoder = &subghz_protocol_baldr_rain_decoder,
    .encoder = &subghz_protocol_baldr_rain_encoder,
};

void* subghz_protocol_encoder_baldr_rain_alloc(SubGhzEnvironment* environment) {
    UNUSED(environment);
    SubGhzProtocolEncoder_baldr_rain* instance = malloc(sizeof(SubGhzProtocolEncoder_baldr_rain));

    instance->base.protocol = &subghz_protocol_baldr_rain;
    instance->generic.protocol_name = instance->base.protocol->name;

    instance->encoder.repeat = 10;
    instance->encoder.size_upload = 52;
    instance->encoder.upload = malloc(instance->encoder.size_upload * sizeof(LevelDuration));
    instance->encoder.is_running = false;
    return instance;
}

void subghz_protocol_encoder_baldr_rain_free(void* context) {
    furi_assert(context);
    SubGhzProtocolEncoder_baldr_rain* instance = context;
    free(instance->encoder.upload);
    free(instance);
}

/**
 * Generating an upload from data.
 * @param instance Pointer to a SubGhzProtocolEncoder_baldr_rain instance
 * @return true On success
 */
static bool subghz_protocol_encoder_baldr_rain_get_upload(SubGhzProtocolEncoder_baldr_rain* instance) {
    furi_assert(instance);
    size_t index = 0;
    size_t size_upload = (instance->generic.data_count_bit * 2) + 2;
    if(size_upload > instance->encoder.size_upload) {
        FURI_LOG_E(TAG, "Size upload exceeds allocated encoder buffer.");
        return false;
    } else {
        instance->encoder.size_upload = size_upload;
    }
    //Send header
    instance->encoder.upload[index++] =
        level_duration_make(false, (uint32_t)subghz_protocol_baldr_rain_const.te_short * 35);
    //Send start bit
    instance->encoder.upload[index++] =
        level_duration_make(true, (uint32_t)subghz_protocol_baldr_rain_const.te_short);
    //Send key data
    for(uint8_t i = instance->generic.data_count_bit; i > 0; i--) {
        if(bit_read(instance->generic.data, i - 1)) {
            //send bit 1
            instance->encoder.upload[index++] =
                level_duration_make(false, (uint32_t)subghz_protocol_baldr_rain_const.te_short);
            instance->encoder.upload[index++] =
                level_duration_make(true, (uint32_t)subghz_protocol_baldr_rain_const.te_long);
        } else {
            //send bit 0
            instance->encoder.upload[index++] =
                level_duration_make(false, (uint32_t)subghz_protocol_baldr_rain_const.te_long);
            instance->encoder.upload[index++] =
                level_duration_make(true, (uint32_t)subghz_protocol_baldr_rain_const.te_short);
        }
    }
    return true;
}

SubGhzProtocolStatus
    subghz_protocol_encoder_baldr_rain_deserialize(void* context, FlipperFormat* flipper_format) {
    furi_assert(context);
    SubGhzProtocolEncoder_baldr_rain* instance = context;
    SubGhzProtocolStatus res = SubGhzProtocolStatusError;
    do {
        res = subghz_block_generic_deserialize_check_count_bit(
            &instance->generic,
            flipper_format,
            subghz_protocol_baldr_rain_const.min_count_bit_for_found);
        if(res != SubGhzProtocolStatusOk) {
            FURI_LOG_E(TAG, "Deserialize error");
            break;
        }
        //optional parameter parameter
        flipper_format_read_uint32(
            flipper_format, "Repeat", (uint32_t*)&instance->encoder.repeat, 1);

        if(!subghz_protocol_encoder_baldr_rain_get_upload(instance)) {
            res = SubGhzProtocolStatusErrorEncoderGetUpload;
            break;
        }
        instance->encoder.is_running = true;
    } while(false);

    return res;
}

void subghz_protocol_encoder_baldr_rain_stop(void* context) {
    SubGhzProtocolEncoder_baldr_rain* instance = context;
    instance->encoder.is_running = false;
}

LevelDuration subghz_protocol_encoder_baldr_rain_yield(void* context) {
    SubGhzProtocolEncoder_baldr_rain* instance = context;

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

void* subghz_protocol_decoder_baldr_rain_alloc(SubGhzEnvironment* environment) {
    UNUSED(environment);
    SubGhzProtocolDecoder_baldr_rain* instance = malloc(sizeof(SubGhzProtocolDecoder_baldr_rain));
    instance->base.protocol = &subghz_protocol_baldr_rain;
    instance->generic.protocol_name = instance->base.protocol->name;
    return instance;
}

void subghz_protocol_decoder_baldr_rain_free(void* context) {
    furi_assert(context);
    SubGhzProtocolDecoder_baldr_rain* instance = context;
    free(instance);
}

void subghz_protocol_decoder_baldr_rain_reset(void* context) {
    furi_assert(context);
    SubGhzProtocolDecoder_baldr_rain* instance = context;
    instance->decoder.parser_step = _baldr_rainDecoderStepReset;
}

void subghz_protocol_decoder_baldr_rain_feed(void* context, bool level, uint32_t duration) {
    furi_assert(context);
    SubGhzProtocolDecoder_baldr_rain* instance = context;

    switch(instance->decoder.parser_step) {
    case _baldr_rainDecoderStepReset:
        if((!level) && (DURATION_DIFF(duration, subghz_protocol_baldr_rain_const.te_short * 35) <
                        subghz_protocol_baldr_rain_const.te_delta * 35)) {
            //Found header _baldr_rain
            instance->decoder.parser_step = _baldr_rainDecoderStepFoundStartBit;
        }
        break;
    case _baldr_rainDecoderStepFoundStartBit:
        if(!level) {
            break;
        } else if(
            DURATION_DIFF(duration, subghz_protocol_baldr_rain_const.te_short) <
            subghz_protocol_baldr_rain_const.te_delta) {
            //Found start bit _baldr_rain
            instance->decoder.parser_step = _baldr_rainDecoderStepSaveDuration;
            instance->decoder.decode_data = 0;
            instance->decoder.decode_count_bit = 0;
        } else {
            instance->decoder.parser_step = _baldr_rainDecoderStepReset;
        }
        break;
    case _baldr_rainDecoderStepSaveDuration:
        if(!level) { //save interval
            if(duration >= (subghz_protocol_baldr_rain_const.te_short * 4)) {
                instance->decoder.parser_step = _baldr_rainDecoderStepFoundStartBit;
                if(instance->decoder.decode_count_bit >=
                   subghz_protocol_baldr_rain_const.min_count_bit_for_found) {
                    instance->generic.serial = 0x0;
                    instance->generic.btn = 0x0;

                    instance->generic.data = instance->decoder.decode_data;
                    instance->generic.data_count_bit = instance->decoder.decode_count_bit;
                    if(instance->base.callback)
                        instance->base.callback(&instance->base, instance->base.context);
                }
                break;
            }
            instance->decoder.te_last = duration;
            instance->decoder.parser_step = _baldr_rainDecoderStepCheckDuration;
        } else {
            instance->decoder.parser_step = _baldr_rainDecoderStepReset;
        }
        break;
    case _baldr_rainDecoderStepCheckDuration:
        if(level) {
            if((DURATION_DIFF(instance->decoder.te_last, subghz_protocol_baldr_rain_const.te_short) <
                subghz_protocol_baldr_rain_const.te_delta) &&
               (DURATION_DIFF(duration, subghz_protocol_baldr_rain_const.te_long) <
                subghz_protocol_baldr_rain_const.te_delta)) {
                subghz_protocol_blocks_add_bit(&instance->decoder, 1);
                instance->decoder.parser_step = _baldr_rainDecoderStepSaveDuration;
            } else if(
                (DURATION_DIFF(instance->decoder.te_last, subghz_protocol_baldr_rain_const.te_long) <
                 subghz_protocol_baldr_rain_const.te_delta) &&
                (DURATION_DIFF(duration, subghz_protocol_baldr_rain_const.te_short) <
                 subghz_protocol_baldr_rain_const.te_delta)) {
                subghz_protocol_blocks_add_bit(&instance->decoder, 0);
                instance->decoder.parser_step = _baldr_rainDecoderStepSaveDuration;
            } else
                instance->decoder.parser_step = _baldr_rainDecoderStepReset;
        } else {
            instance->decoder.parser_step = _baldr_rainDecoderStepReset;
        }
        break;
    }
}

/** 
 * Analysis of received data
 * @param instance Pointer to a SubGhzBlockGeneric* instance
 */
static void subghz_protocol_baldr_rain_check_remote_controller(SubGhzBlockGeneric* instance) {
    /*
 *        12345678(10) k   9                    
 * AAA => 10101010 1   01  0
 *
 * 1...10 - DIP
 * k- KEY
 */
    instance->cnt = instance->data & 0xFFF;
    instance->btn = ((instance->data >> 1) & 0x3);
}

uint8_t subghz_protocol_decoder_baldr_rain_get_hash_data(void* context) {
    furi_assert(context);
    SubGhzProtocolDecoder_baldr_rain* instance = context;
    return subghz_protocol_blocks_get_hash_data(
        &instance->decoder, (instance->decoder.decode_count_bit / 8) + 1);
}

SubGhzProtocolStatus subghz_protocol_decoder_baldr_rain_serialize(
    void* context,
    FlipperFormat* flipper_format,
    SubGhzRadioPreset* preset) {
    furi_assert(context);
    SubGhzProtocolDecoder_baldr_rain* instance = context;
    return subghz_block_generic_serialize(&instance->generic, flipper_format, preset);
}

SubGhzProtocolStatus
    subghz_protocol_decoder_baldr_rain_deserialize(void* context, FlipperFormat* flipper_format) {
    furi_assert(context);
    SubGhzProtocolDecoder_baldr_rain* instance = context;
    return subghz_block_generic_deserialize_check_count_bit(
        &instance->generic, flipper_format, subghz_protocol_baldr_rain_const.min_count_bit_for_found);
}

void subghz_protocol_decoder_baldr_rain_get_string(void* context, FuriString* output) {
    furi_assert(context);
    SubGhzProtocolDecoder_baldr_rain* instance = context;
    subghz_protocol_baldr_rain_check_remote_controller(&instance->generic);
    furi_string_cat_printf(
        output,
        "%s %dbit\r\n"
        "Key:%03lX\r\n"
        "Btn:%X\r\n",
//        "DIP:" DIP_PATTERN "\r\n",
        instance->generic.protocol_name,
        instance->generic.data_count_bit,
        (uint32_t)(instance->generic.data & 0xFFFFFFFF),
        instance->generic.btn); //,
//        CNT_TO_DIP(instance->generic.cnt));
}
