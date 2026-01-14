#ifndef BASE64_H
#define BASE64_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Decode a Base64 string into a buffer
// Returns number of bytes written to output, or -1 on invalid input
int base64_decode(const char *input, unsigned char *output, size_t out_size);

// Encode a buffer to Base64 string
// Returns number of bytes written to output, or -1 if output buffer too small
int base64_encode(const unsigned char *input, size_t in_len, char *output, size_t out_size);

#ifdef __cplusplus
}
#endif

#endif // BASE64_H

