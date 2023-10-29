#pragma once

#include "../types.h"

#ifdef RTL_433_REDUCE_STACK_USE
#define BITBUF_COLS 40 // Number of bytes in a column
#define BITBUF_ROWS 25
#else
#define BITBUF_COLS 128 // Number of bytes in a column
#define BITBUF_ROWS 50
#endif
#define BITBUF_MAX_ROW_BITS (BITBUF_ROWS * BITBUF_COLS * 8) // Maximum number of bits per row, max UINT16_MAX
#define BITBUF_MAX_PRINT_BITS 50 // Maximum number of bits to print (in addition to hex values)


#ifdef __cplusplus
extern "C" {
#endif

typedef struct SubGhzProtocolDecoderBase SubGhzProtocolDecoderBase;

typedef void (
    *SubGhzProtocolDecoderBaseRxCallback)(SubGhzProtocolDecoderBase* instance, void* context);

typedef void (*SubGhzProtocolDecoderBaseSerialize)(
    SubGhzProtocolDecoderBase* decoder_base,
    FuriString* output);

typedef uint8_t bitrow_t[BITBUF_COLS];
typedef bitrow_t bitarray_t[BITBUF_ROWS];

enum decode_return_codes {
    DECODE_FAIL_OTHER   = 0, ///< legacy, do not use
    /** Bitbuffer row count or row length is wrong for this sensor. */
    DECODE_ABORT_LENGTH = -1,
    DECODE_ABORT_EARLY  = -2,
    /** Message Integrity Check failed: e.g. checksum/CRC doesn't validate. */
    DECODE_FAIL_MIC     = -3,
    DECODE_FAIL_SANITY  = -4,
};

typedef enum {
    DATA_DATA,   /**< pointer to data is stored */
    DATA_INT,    /**< pointer to integer is stored */
    DATA_DOUBLE, /**< pointer to a double is stored */
    DATA_STRING, /**< pointer to a string is stored */
    DATA_ARRAY,  /**< pointer to an array of values is stored */
    DATA_COUNT,  /**< invalid */
    DATA_FORMAT, /**< indicates the following value is formatted */
    DATA_COND,   /**< add data only if condition is true, skip otherwise */
} data_type_t;

typedef struct data_array {
    int         num_values;
    data_type_t type;
    void        *values;
} data_array_t;

typedef union data_value {
    int         v_int;  /**< A data value of type int, 4 bytes size/alignment */
    double      v_dbl;  /**< A data value of type double, 8 bytes size/alignment */
    void        *v_ptr; /**< A data value pointer, 4/8 bytes size/alignment */
} data_value_t;

typedef struct bitbuffer {
    uint16_t num_rows;                      ///< Number of active rows
    uint16_t free_row;                      ///< Index of next free row
    uint16_t bits_per_row[BITBUF_ROWS];     ///< Number of active bits per row
    uint16_t syncs_before_row[BITBUF_ROWS]; ///< Number of sync pulses before row
    bitarray_t bb;                          ///< The actual bits buffer
} bitbuffer_t;

void bitbuffer_invert(bitbuffer_t *bits);

typedef struct data {
    struct data *next; /**< chaining to the next element in the linked list; NULL indicates end-of-list */
    char        *key;
    char        *pretty_key; /**< the name used for displaying data to user in with a nicer name */
    char        *format; /**< if not null, contains special formatting string */
    data_value_t value;
    data_type_t type;
    unsigned    retain; /**< incremented on data_retain, data_free only frees if this is zero */
} data_t;

int add_bytes(uint8_t const message[], unsigned num_bytes);
//void decoder_output_data(SubGhzProtocolDecoderBase *decoder, data_t *data);

struct SubGhzProtocolDecoderBase {
    // Decoder general section
    const SubGhzProtocol* protocol;
	
	int row; // Aggiungi una variabile per il conteggio delle righe
    bitbuffer_t *bitbuffer; // Aggiungi una variabile per il bitbuffer
	
    // Callback section
    SubGhzProtocolDecoderBaseRxCallback callback;
    void* context;
};

void bitbuffer_extract_bytes(bitbuffer_t *bitbuffer, unsigned row, unsigned pos, uint8_t *out, unsigned len);

/**
 * Set a callback upon completion of successful decoding of one of the protocols.
 * @param decoder_base Pointer to a SubGhzProtocolDecoderBase instance
 * @param callback Callback, SubGhzProtocolDecoderBaseRxCallback
 * @param context Context
 */
void subghz_protocol_decoder_base_set_decoder_callback(
    SubGhzProtocolDecoderBase* decoder_base,
    SubGhzProtocolDecoderBaseRxCallback callback,
    void* context);

/**
 * Getting a textual representation of the received data.
 * @param decoder_base Pointer to a SubGhzProtocolDecoderBase instance
 * @param output Resulting text
 */
bool subghz_protocol_decoder_base_get_string(
    SubGhzProtocolDecoderBase* decoder_base,
    FuriString* output);

/**
 * Serialize data SubGhzProtocolDecoderBase.
 * @param decoder_base Pointer to a SubGhzProtocolDecoderBase instance
 * @param flipper_format Pointer to a FlipperFormat instance
 * @param preset The modulation on which the signal was received, SubGhzRadioPreset
 * @return Status Error
 */
SubGhzProtocolStatus subghz_protocol_decoder_base_serialize(
    SubGhzProtocolDecoderBase* decoder_base,
    FlipperFormat* flipper_format,
    SubGhzRadioPreset* preset);

/**
 * Deserialize data SubGhzProtocolDecoderBase.
 * @param decoder_base Pointer to a SubGhzProtocolDecoderBase instance
 * @param flipper_format Pointer to a FlipperFormat instance
 * @return Status Error
 */
SubGhzProtocolStatus subghz_protocol_decoder_base_deserialize(
    SubGhzProtocolDecoderBase* decoder_base,
    FlipperFormat* flipper_format);

/**
 * Getting the hash sum of the last randomly received parcel.
 * @param decoder_base Pointer to a SubGhzProtocolDecoderBase instance
 * @return hash Hash sum
 */
uint8_t subghz_protocol_decoder_base_get_hash_data(SubGhzProtocolDecoderBase* decoder_base);

// Encoder Base
typedef struct SubGhzProtocolEncoderBase SubGhzProtocolEncoderBase;

struct SubGhzProtocolEncoderBase {
    // Decoder general section
    const SubGhzProtocol* protocol;

    // Callback section
};

#ifdef __cplusplus
}
#endif
