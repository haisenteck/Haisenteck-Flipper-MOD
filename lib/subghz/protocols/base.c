#include "base.h"
#include "registry.h"

void subghz_protocol_decoder_base_set_decoder_callback(
    SubGhzProtocolDecoderBase* decoder_base,
    SubGhzProtocolDecoderBaseRxCallback callback,
    void* context) {
    decoder_base->callback = callback;
    decoder_base->context = context;
}

bool subghz_protocol_decoder_base_get_string(
    SubGhzProtocolDecoderBase* decoder_base,
    FuriString* output) {
    bool status = false;

    if(decoder_base->protocol && decoder_base->protocol->decoder &&
       decoder_base->protocol->decoder->get_string) {
        decoder_base->protocol->decoder->get_string(decoder_base, output);
        status = true;
    }

    return status;
}

SubGhzProtocolStatus subghz_protocol_decoder_base_serialize(
    SubGhzProtocolDecoderBase* decoder_base,
    FlipperFormat* flipper_format,
    SubGhzRadioPreset* preset) {
    SubGhzProtocolStatus status = SubGhzProtocolStatusError;

    if(decoder_base->protocol && decoder_base->protocol->decoder &&
       decoder_base->protocol->decoder->serialize) {
        status = decoder_base->protocol->decoder->serialize(decoder_base, flipper_format, preset);
    }

    return status;
}

SubGhzProtocolStatus subghz_protocol_decoder_base_deserialize(
    SubGhzProtocolDecoderBase* decoder_base,
    FlipperFormat* flipper_format) {
    SubGhzProtocolStatus status = SubGhzProtocolStatusError;

    if(decoder_base->protocol && decoder_base->protocol->decoder &&
       decoder_base->protocol->decoder->deserialize) {
        status = decoder_base->protocol->decoder->deserialize(decoder_base, flipper_format);
    }

    return status;
}

uint8_t subghz_protocol_decoder_base_get_hash_data(SubGhzProtocolDecoderBase* decoder_base) {
    uint8_t hash = 0;

    if(decoder_base->protocol && decoder_base->protocol->decoder &&
       decoder_base->protocol->decoder->get_hash_data) {
        hash = decoder_base->protocol->decoder->get_hash_data(decoder_base);
    }

    return hash;
}

void bitbuffer_extract_bytes(bitbuffer_t *bitbuffer, unsigned row, unsigned pos, uint8_t *out, unsigned len)
{
    uint8_t *bits = bitbuffer->bb[row];
    if (len == 0)
        return;
    if ((pos & 7) == 0) {
        memcpy(out, bits + (pos / 8), (len + 7) / 8);
    }
    else {
        unsigned shift = 8 - (pos & 7);
        unsigned bytes = (len + 7) >> 3;
        uint8_t *p = out;
        uint16_t word;
        pos = pos >> 3; // Convert to bytes

        word = bits[pos];

        while (bytes--) {
            word <<= 8;
            word |= bits[++pos];
            *(p++) = word >> shift;
        }
    }
    if (len & 7)
        out[(len - 1) / 8] &= 0xff00 >> (len & 7); // mask off bottom bits
}

int add_bytes(uint8_t const message[], unsigned num_bytes)
{
    int result = 0;
    for (unsigned i = 0; i < num_bytes; ++i) {
        result += message[i];
    }
    return result;
}

void bitbuffer_invert(bitbuffer_t *bits)
{
    for (unsigned row = 0; row < bits->num_rows; ++row) {
        if (bits->bits_per_row[row] > 0) {
            uint8_t *b = bits->bb[row];

            const unsigned last_col  = (bits->bits_per_row[row] - 1) / 8;
            const unsigned last_bits = ((bits->bits_per_row[row] - 1) % 8) + 1;
            for (unsigned col = 0; col <= last_col; ++col) {
                b[col] = ~b[col]; // Invert
            }
            b[last_col] ^= 0xFF >> last_bits; // Re-invert unused bits in last byte
        }
    }
}