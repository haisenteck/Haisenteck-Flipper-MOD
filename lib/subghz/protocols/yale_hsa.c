#include "yale_hsa.h"

#include "../blocks/const.h"
#include "../blocks/decoder.h"
#include "../blocks/encoder.h"
#include "../blocks/generic.h"
#include "../blocks/math.h"

#define TAG "YaleHSA"

static const SubGhzBlockConst subghz_protocol_yale_hsa_const = {
    .te_short = 850,
    .te_long = 1460,
    .te_delta = 150,
    .min_count_bit_for_found = 13,
};

struct SubGhzProtocolDecoderyale_hsa {
    SubGhzProtocolDecoderBase base;

    SubGhzBlockDecoder decoder;
    SubGhzBlockGeneric generic;
};

struct SubGhzProtocolEncoderyale_hsa {
    SubGhzProtocolEncoderBase base;

    SubGhzProtocolBlockEncoder encoder;
    SubGhzBlockGeneric generic;
};

typedef enum {
    yale_hsaDecoderStepReset = 0,
    yale_hsaDecoderStepSaveDuration,
    yale_hsaDecoderStepCheckDuration,
} yale_hsaDecoderStep;

const SubGhzProtocolDecoder subghz_protocol_yale_hsa_decoder = {
    .alloc = subghz_protocol_decoder_yale_hsa_alloc,
    .free = subghz_protocol_decoder_yale_hsa_free,

    .feed = subghz_protocol_decoder_yale_hsa_feed,
    .reset = subghz_protocol_decoder_yale_hsa_reset,

    .get_hash_data = subghz_protocol_decoder_yale_hsa_get_hash_data,
    .serialize = subghz_protocol_decoder_yale_hsa_serialize,
    .deserialize = subghz_protocol_decoder_yale_hsa_deserialize,
    .get_string = subghz_protocol_decoder_yale_hsa_get_string,
};

const SubGhzProtocolEncoder subghz_protocol_yale_hsa_encoder = {
    .alloc = NULL,
    .free = NULL,

    .deserialize = NULL,
    .stop = NULL,
    .yield = NULL,
};

const SubGhzProtocol subghz_protocol_yale_hsa = {
    .name = SUBGHZ_PROTOCOL_YALE_HSA_NAME,
    .type = SubGhzProtocolTypeStatic,
    .flag = SubGhzProtocolFlag_433 | SubGhzProtocolFlag_AM | SubGhzProtocolFlag_Decodable |
            SubGhzProtocolFlag_Load | SubGhzProtocolFlag_Save | SubGhzProtocolFlag_Send,

    .decoder = &subghz_protocol_yale_hsa_decoder,
    .encoder = &subghz_protocol_yale_hsa_encoder,
};

void *subghz_protocol_decoder_yale_hsa_alloc(SubGhzEnvironment *environment) {
    UNUSED(environment);
    SubGhzProtocolDecoderyale_hsa *instance = malloc(sizeof(SubGhzProtocolDecoderyale_hsa));
    instance->base.protocol = &subghz_protocol_yale_hsa;
    instance->generic.protocol_name = instance->base.protocol->name;
    return instance;
}

void subghz_protocol_decoder_yale_hsa_free(void *context) {
    furi_assert(context);
    SubGhzProtocolDecoderyale_hsa *instance = context;
    free(instance);
}

void subghz_protocol_decoder_yale_hsa_reset(void *context) {
    furi_assert(context);
    SubGhzProtocolDecoderyale_hsa *instance = context;
    instance->decoder.parser_step = yale_hsaDecoderStepReset;
}

//static int subghz_protocol_decoder_yale_hsa_feed_bit(SubGhzProtocolDecoderyale_hsa *instance);
 
static int subghz_protocol_decoder_yale_hsa_feed_bit(SubGhzProtocolDecoderyale_hsa *instance, int bit) {
    UNUSED(bit); // Ignora il parametro bit non utilizzat
	// Require at least 6 rows
    if (instance->base.bitbuffer->num_rows < 6)
        return 0;

    uint8_t msg[6] = {0};
    for (int row = 0; row < instance->base.bitbuffer->num_rows; ++row) {
        // Find one full message
        int ok = 0;
        for (int i = 0; i < 6; ++i, ++row) {
            if (instance->base.bitbuffer->bits_per_row[row] != 13)
                break; // wrong length
            uint8_t *b = instance->base.bitbuffer->bb[row];
            if ((b[0] & 0xf0) != 0x50)
                break; // wrong sync
            int eom = (b[0] & 0x08);
            if ((i < 5 && eom) || (i == 5 && !eom))
                break; // wrong end-of-message
            bitbuffer_extract_bytes(instance->base.bitbuffer, row, 5, &msg[i], 8);
            if (i == 5)
                ok = 1;
        }
        // Skip to end-of-message on error
        if (!ok) {
            for (; row < instance->base.bitbuffer->num_rows; ++row) {
                uint8_t *b = instance->base.bitbuffer->bb[row];
                int eom = (b[0] & 0x08);
                if (eom)
                    break; // end-of-message
            }
            continue;
        }
        // Message found
        int chk = add_bytes(msg, 6);
        if (chk & 0xff)
            continue; // bad checksum

        /* // Get the data
        int id = (msg[0] << 8) | (msg[1]);
        int stype = (msg[2]);
        int state = (msg[3]);
        int event = (msg[4]);

        //clang-format off 
        data_t *data = data_make(
                "model",        "",             DATA_STRING, "Yale-HSA",
                "id",           "",             DATA_FORMAT, "%04x", DATA_INT, id,
                "stype",        "Sensor type",  DATA_FORMAT, "%02x", DATA_INT, stype,
                "state",        "State",        DATA_FORMAT, "%02x", DATA_INT, state,
                "event",        "Event",        DATA_FORMAT, "%02x", DATA_INT, event,
                "mic",          "Integrity",    DATA_STRING, "CHECKSUM",
                NULL);
        // clang-format on

        decoder_output_data(&instance->decoder, data); 
		*/
        return 1;
    }
    return 0;
}

void subghz_protocol_decoder_yale_hsa_feed(void *context, bool level, uint32_t duration) {
    furi_assert(context);
    SubGhzProtocolDecoderyale_hsa *instance = context;

    switch (instance->decoder.parser_step) {
    case yale_hsaDecoderStepReset:
        if ((!level) && (DURATION_DIFF(duration, subghz_protocol_yale_hsa_const.te_short * 44) <
                         (subghz_protocol_yale_hsa_const.te_delta * 15))) {
            instance->decoder.decode_data = 0;
            instance->decoder.decode_count_bit = 0;
            instance->decoder.parser_step = yale_hsaDecoderStepCheckDuration;
        }
        break;
    case yale_hsaDecoderStepSaveDuration:
        if (!level) {
            if (DURATION_DIFF(duration, subghz_protocol_yale_hsa_const.te_short * 44) <
                (subghz_protocol_yale_hsa_const.te_delta * 15)) {
                if (instance->decoder.decode_count_bit ==
                    subghz_protocol_yale_hsa_const.min_count_bit_for_found) {
                    instance->generic.data = instance->decoder.decode_data;
                    instance->generic.data_count_bit = instance->decoder.decode_count_bit;

                    if (instance->base.callback)
                        instance->base.callback(&instance->base, instance->base.context);
                } else {
                    instance->decoder.parser_step = yale_hsaDecoderStepReset;
                }
                instance->decoder.decode_data = 0;
                instance->decoder.decode_count_bit = 0;
                break;
            } else {
                if ((DURATION_DIFF(duration, subghz_protocol_yale_hsa_const.te_short) <
                     subghz_protocol_yale_hsa_const.te_delta) ||
                    (DURATION_DIFF(duration, subghz_protocol_yale_hsa_const.te_long) <
                     subghz_protocol_yale_hsa_const.te_delta * 3)) {
                    instance->decoder.parser_step = yale_hsaDecoderStepCheckDuration;
                } else {
                    instance->decoder.parser_step = yale_hsaDecoderStepReset;
                }
            }
        }
        break;
    case yale_hsaDecoderStepCheckDuration:
        if (level) {
            if (DURATION_DIFF(duration, subghz_protocol_yale_hsa_const.te_long) <
                subghz_protocol_yale_hsa_const.te_delta * 3) {
                subghz_protocol_decoder_yale_hsa_feed_bit(instance, 1);
                instance->decoder.parser_step = yale_hsaDecoderStepSaveDuration;
            } else if (
                DURATION_DIFF(duration, subghz_protocol_yale_hsa_const.te_short) <
                subghz_protocol_yale_hsa_const.te_delta) {
                subghz_protocol_decoder_yale_hsa_feed_bit(instance, 0);
                instance->decoder.parser_step = yale_hsaDecoderStepSaveDuration;
            } else {
                instance->decoder.parser_step = yale_hsaDecoderStepReset;
            }
        } else {
            instance->decoder.parser_step = yale_hsaDecoderStepReset;
        }
        break;
    }
}

uint8_t subghz_protocol_decoder_yale_hsa_get_hash_data(void *context) {
    furi_assert(context);
    SubGhzProtocolDecoderyale_hsa *instance = context;
    return subghz_protocol_blocks_get_hash_data(
        &instance->decoder, (instance->decoder.decode_count_bit / 8) + 1);
}

SubGhzProtocolStatus subghz_protocol_decoder_yale_hsa_serialize(
    void *context,
    FlipperFormat *flipper_format,
    SubGhzRadioPreset *preset) {
    furi_assert(context);
    SubGhzProtocolDecoderyale_hsa *instance = context;
    return subghz_block_generic_serialize(&instance->generic, flipper_format, preset);
}

SubGhzProtocolStatus subghz_protocol_decoder_yale_hsa_deserialize(void *context, FlipperFormat *flipper_format) {
    furi_assert(context);
    SubGhzProtocolDecoderyale_hsa *instance = context;
    return subghz_block_generic_deserialize_check_count_bit(
        &instance->generic, flipper_format, subghz_protocol_yale_hsa_const.min_count_bit_for_found);
}

void subghz_protocol_decoder_yale_hsa_get_string(void *context, FuriString *output) {
    furi_assert(context);
    SubGhzProtocolDecoderyale_hsa *instance = context;
    uint32_t data = (uint32_t)(instance->generic.data & 0x3FFFF);
    furi_string_cat_printf(
        output,
        "%s %dbit\r\n"
        "Key:%05lX\r\n"
        //"  +:   " DIP_PATTERN "\r\n"
        //"  o:   " DIP_PATTERN "\r\n"
        //"  -:   " DIP_PATTERN "\r\n"
        ,
        instance->generic.protocol_name,
        instance->generic.data_count_bit,
        data
        //,
        //SHOW_DIP_P(data, DIP_P),
        //SHOW_DIP_P(data, DIP_O),
        //SHOW_DIP_P(data, DIP_N)
    );
}
