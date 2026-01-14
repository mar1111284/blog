#include "base64.h"
#include <string.h>

static unsigned char decoding_table[256];
static char encoding_table[64] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// Initialize decoding table once
static void init_decoding_table() {
    static int initialized = 0;
    if(initialized) return;

    for(int i = 0; i < 256; i++) decoding_table[i] = 0x80; // invalid marker

    for(int i = 0; i < 26; i++) {
        decoding_table['A'+i] = i;
        decoding_table['a'+i] = i + 26;
    }
    for(int i = 0; i < 10; i++) decoding_table['0'+i] = i + 52;
    decoding_table['+'] = 62;
    decoding_table['/'] = 63;
    decoding_table['='] = 0;

    initialized = 1;
}

int base64_decode(const char *input, unsigned char *output, size_t out_size) {
    init_decoding_table();

    size_t len = strlen(input);
    size_t out_index = 0;

    if(len % 4 != 0) return -1; // invalid base64 length

    for(size_t i = 0; i < len; i += 4) {
        unsigned char c0 = decoding_table[(unsigned char)input[i]];
        unsigned char c1 = decoding_table[(unsigned char)input[i+1]];
        unsigned char c2 = decoding_table[(unsigned char)input[i+2]];
        unsigned char c3 = decoding_table[(unsigned char)input[i+3]];

        if(c0 & 0x80 || c1 & 0x80 || c2 & 0x80 || c3 & 0x80) return -1;

        if(out_index + 3 > out_size) return -1; // prevent overflow

        output[out_index++] = (c0 << 2) | (c1 >> 4);
        if(input[i+2] != '=') output[out_index++] = (c1 << 4) | (c2 >> 2);
        if(input[i+3] != '=') output[out_index++] = (c2 << 6) | c3;
    }

    return (int)out_index;
}

int base64_encode(const unsigned char *input, size_t in_len, char *output, size_t out_size) {
    size_t out_index = 0;

    for(size_t i = 0; i < in_len; i += 3) {
        unsigned int b0 = input[i];
        unsigned int b1 = (i+1 < in_len) ? input[i+1] : 0;
        unsigned int b2 = (i+2 < in_len) ? input[i+2] : 0;

        if(out_index + 4 > out_size) return -1; // prevent overflow

        output[out_index++] = encoding_table[(b0 >> 2) & 0x3F];
        output[out_index++] = encoding_table[((b0 & 0x3) << 4) | ((b1 >> 4) & 0xF)];
        if(i+1 < in_len) output[out_index++] = encoding_table[((b1 & 0xF) << 2) | ((b2 >> 6) & 0x3)];
        else output[out_index++] = '=';
        if(i+2 < in_len) output[out_index++] = encoding_table[b2 & 0x3F];
        else output[out_index++] = '=';
    }

    if(out_index < out_size) output[out_index] = '\0';
    return (int)out_index;
}

