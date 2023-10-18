#include "calibeur.h"

#define TAG "subghz_protocol_calibeur"

static const SubGhzBlockConst subghz_protocol_calibeur_const = {
    .te_short = 760,     // Short pulse 760µs
    .te_long = 2240,     // Long pulse 2240µs
    .te_delta = 2960,    // Pulse rate 2960µs
    .min_count_bit_for_found = 111, // Total bit count for a valid transmission
};

struct subghz_protocol_calibeur_decoder {
    SubGhzProtocolDecoderBase base;

    SubGhzBlockDecoder decoder;
    SubGhzBlockGeneric generic;
};

struct SubGhzProtocolEncoderCalibeur {
    SubGhzProtocolEncoderBase base;

    SubGhzProtocolBlockEncoder encoder;
    SubGhzBlockGeneric generic;
};

typedef enum {
    calibeurDecoderStepReset = 0,
    calibeurDecoderStepSaveDuration,
    calibeurDecoderStepCheckDuration,
} CalibeurDecoderStep;

const SubGhzProtocolDecoder SubGhzProtocolDecoderCalibeur = {
    .alloc = subghz_protocol_decoder_calibeur_alloc,
    .free = subghz_protocol_decoder_calibeur_free,

    .feed = subghz_protocol_decoder_calibeur_feed,
    .reset = subghz_protocol_decoder_calibeur_reset,

    .get_hash_data = subghz_protocol_decoder_calibeur_get_hash_data,
    .serialize = subghz_protocol_decoder_calibeur_serialize,
    .deserialize = subghz_protocol_decoder_calibeur_deserialize,
    .get_string = subghz_protocol_decoder_calibeur_get_string,
};

void subghz_protocol_decoder_base_set_decoder_callback(
    SubGhzProtocolDecoderBase* decoder_base,
    SubGhzProtocolDecoderBaseRxCallback callback,
    void* context) {
    decoder_base->callback = callback;
    decoder_base->context = context;
}

void* subghz_protocol_decoder_calibeur_alloc(SubGhzEnvironment* environment) {
    UNUSED(environment);
    subghz_protocol_calibeur_decoder* instance = malloc(sizeof(subghz_protocol_decoder_calibeur));
    instance->base.protocol = &subghz_protocol_calibeur;
    instance->generic.protocol_name = instance->base.protocol->name;
    return instance;
}

void subghz_protocol_decoder_calibeur_free(void* context) {
    furi_assert(context);
    subghz_protocol_decoder_calibeur* instance = context;
    free(instance);
}

void subghz_protocol_decoder_calibeur_reset(void* context) {
    furi_assert(context);
    subghz_protocol_decoder_calibeur* instance = context;
    instance->decoder.parser_step = calibeurDecoderStepReset;
}

uint8_t subghz_protocol_decoder_calibeur_get_hash_data(void* context) {
    furi_assert(context);
    subghz_protocol_decoder_calibeur* instance = context;
    return subghz_protocol_blocks_get_hash_data(
        &instance->decoder, (instance->decoder.decode_count_bit / 8) + 1);
}

SubGhzProtocolStatus subghz_protocol_decoder_calibeur_serialize(
    void* context,
    FlipperFormat* flipper_format,
    SubGhzRadioPreset* preset) {
    furi_assert(context);
    subghz_protocol_decoder_calibeur* instance = context;
    return subghz_block_generic_serialize(&instance->generic, flipper_format, preset);
}

SubGhzProtocolStatus subghz_protocol_decoder_calibeur_deserialize(void* context, FlipperFormat* flipper_format) {
    furi_assert(context);
    subghz_protocol_decoder_calibeur* instance = context;
    return subghz_block_generic_deserialize_check_count_bit(
        &instance->generic, flipper_format, subghz_protocol_calibeur_const.min_count_bit_for_found);
}


static int calibeur_rf104_decode(SubGhzProtocolDecoderBase* decoder_base, bitbuffer_t* bitbuffer)
{
    subghz_protocol_decoder_calibeur* instance = (subghz_protocol_decoder_calibeur)decoder_base;
    uint8_t id;
    float temperature;
    float humidity;
    uint8_t* b = bitbuffer->bb[1];
    uint8_t* b2 = bitbuffer->bb[2];

    // row [0] is empty due to sync bit
    // No need to decode/extract values for simple test
    // check for 0x00 and 0xff
    if ((!b[0] && !b[1] && !b[2]) ||
        (b[0] == 0xff && b[1] == 0xff && b[2] == 0xff)) {
        subghz_protocol_decoder_base_invoke_callback(decoder_base, SubGhzProtocolDecoderBaseEventDecodingFailed, NULL);
        return DECODE_FAIL_SANITY;
    }

    bitbuffer_invert(bitbuffer);
    // Validate package (row [0] is empty due to sync bit)
    if (bitbuffer->bits_per_row[1] != 21) // Don't waste time on a long/short package
        return DECODE_ABORT_LENGTH;
    if (crc8(b, 3, 0x80, 0) == 0) // It should be odd parity
        return DECODE_FAIL_MIC;
    if ((b[0] != b2[0]) || (b[1] != b2[1]) || (b[2] != b2[2]) // We want at least two messages in a row
        return DECODE_FAIL_SANITY;

    uint8_t bits;

    bits = ((b[0] & 0x80) >> 7);   // [0]
    bits |= ((b[0] & 0x40) >> 5);   // [1]
    bits |= ((b[0] & 0x20) >> 3);   // [2]
    bits |= ((b[0] & 0x10) >> 1);   // [3]
    bits |= ((b[0] & 0x08) << 1);   // [4]
    bits |= ((b[0] & 0x04) << 3);   // [5]
    id = bits / 10;
    temperature = (float)(bits % 10) * 0.1f;

    bits = ((b[0] & 0x02) << 3);   // [4]
    bits |= ((b[0] & 0x01) << 5);   // [5]
    bits |= ((b[1] & 0x80) >> 7);   // [0]
    bits |= ((b[1] & 0x40) >> 5);   // [1]
    bits |= ((b[1] & 0x20) >> 3);   // [2]
    bits |= ((b[1] & 0x10) >> 1);   // [3]
    bits |= ((b[1] & 0x08) << 3);   // [6]
    temperature += (float)bits - 41.0f;

    bits = ((b[1] & 0x02) << 4);   // [5]
    bits |= ((b[1] & 0x01) << 6);   // [6]
    bits |= ((b[2] & 0x80) >> 7);   // [0]
    bits |= ((b[2] & 0x40) >> 5);   // [1]
    bits |= ((b[2] & 0x20) >> 3);   // [2]
    bits |= ((b[2] & 0x10) >> 1);   // [3]
    bits |= ((b[2] & 0x08) << 1);   // [4]
    humidity = bits;

    SubGhzProtocolDecoderBaseEventDecodedData* decoded_data = (SubGhzProtocolDecoderBaseEventDecodedData*)malloc(sizeof(SubGhzProtocolDecoderBaseEventDecodedData));
    if (decoded_data) {
        decoded_data->data = calloc(1, sizeof(SubGhzProtocolData));
        decoded_data->data->model = "Calibeur-RF104";
        decoded_data->data->id = id;
        decoded_data->data->temperature_C = temperature;
        decoded_data->data->humidity = humidity;
        decoded_data->data->mic = "CRC";
        subghz_protocol_decoder_base_invoke_callback(decoder_base, SubGhzProtocolDecoderBaseEventDecoded, decoded_data);
        free(decoded_data->data);
        free(decoded_data);
        return 1;
    } else {
        return DECODE_FAIL_MEM;
    }
}

SubGhzProtocolStatus subghz_protocol_decoder_calibeur_feed(
    void* context,
    SubGhzBlockDecoder* decoder,
    SubGhzRadioPreset* preset,
    SubGhzRadioOutput* output) {
    furi_assert(context);
    furi_assert(decoder);
    furi_assert(preset);
    furi_assert(output);

    subghz_protocol_decoder_calibeur* instance = context;

    bitbuffer_t bitbuffer;
    bitbuffer.bb[1] = decoder->bb[1];
    bitbuffer.bb[2] = decoder->bb[2];
    bitbuffer.bits_per_row[1] = decoder->bits_per_row[1];
    bitbuffer.bits_per_row[2] = decoder->bits_per_row[2];

    uint8_t id;
    float temperature;
    float humidity;

    // row [0] is empty due to sync bit
    // No need to decode/extract values for a simple test
    // check for 0x00 and 0xff
    if ((!bitbuffer.bb[1][0] && !bitbuffer.bb[1][1] && !bitbuffer.bb[1][2]) ||
        (bitbuffer.bb[1][0] == 0xff && bitbuffer.bb[1][1] == 0xff && bitbuffer.bb[1][2] == 0xff)) {
        return SubGhzProtocolStatusError;
    }

    bitbuffer_invert(&bitbuffer);

    // Validate package (row [0] is empty due to sync bit)
    if (bitbuffer.bits_per_row[1] != 21) {
        return SubGhzProtocolStatusError;
    }

    if (crc8(bitbuffer.bb[1], 3, 0x80, 0) == 0) {
        return SubGhzProtocolStatusError;
    }

    if ((bitbuffer.bb[1][0] != bitbuffer.bb[2][0]) || (bitbuffer.bb[1][1] != bitbuffer.bb[2][1]) ||
        (bitbuffer.bb[1][2] != bitbuffer.bb[2][2])) {
        return SubGhzProtocolStatusError;
    }

    uint8_t bits = ((bitbuffer.bb[1][0] & 0x80) >> 7);   // [0]
    bits |= ((bitbuffer.bb[1][0] & 0x40) >> 5);   // [1]
    bits |= ((bitbuffer.bb[1][0] & 0x20) >> 3);   // [2]
    bits |= ((bitbuffer.bb[1][0] & 0x10) >> 1);   // [3]
    bits |= ((bitbuffer.bb[1][0] & 0x08) << 1);   // [4]
    bits |= ((bitbuffer.bb[1][0] & 0x04) << 3);   // [5]
    id = bits / 10;
    temperature = (float)(bits % 10) * 0.1f;

    bits = ((bitbuffer.bb[1][0] & 0x02) << 3);   // [4]
    bits |= ((bitbuffer.bb[1][0] & 0x01) << 5);   // [5]
    bits |= ((bitbuffer.bb[1][1] & 0x80) >> 7);   // [0]
    bits |= ((bitbuffer.bb[1][1] & 0x40) >> 5);   // [1]
    bits |= ((bitbuffer.bb[1][1] & 0x20) >> 3);   // [2]
    bits |= ((bitbuffer.bb[1][1] & 0x10) >> 1);   // [3]
    bits |= ((bitbuffer.bb[1][1] & 0x08) << 3);   // [6]
    temperature += (float)bits - 41.0f;

    bits = ((bitbuffer.bb[1][1] & 0x02) << 4);   // [5]
    bits |= ((bitbuffer.bb[1][1] & 0x01) << 6);   // [6]
    bits |= ((bitbuffer.bb[1][2] & 0x80) >> 7);   // [0]
    bits |= ((bitbuffer.bb[1][2] & 0x40) >> 5);   // [1]
    bits |= ((bitbuffer.bb[1][2] & 0x20) >> 3);   // [2]
    bits |= ((bitbuffer.bb[1][2] & 0x10) >> 1);   // [3]
    bits |= ((bitbuffer.bb[1][2] & 0x08) << 1);   // [4]
    humidity = bits;

    // Create and populate the output structure
    output->model = "Calibeur-RF104";
    output->id = id;
    output->temperature_C = temperature;
    output->humidity = humidity;
    output->mic = "CRC";

    return SubGhzProtocolStatusSuccess;
}


void subghz_protocol_decoder_calibeur_get_string(void* context, FuriString* output) {
    furi_assert(context);
    subghz_protocol_decoder_calibeur* instance = context;
    furi_string_printf(
        output,
        "%s %dbit\r\n"
        "Key:0x%lX%08lX\r\n"
        "Sn:%d Ch:%d  Bat:%d\r\n"
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
