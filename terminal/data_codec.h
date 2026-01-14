#ifndef DATA_CODEC_H
#define DATA_CODEC_H

#include <stddef.h>

// Pointer to the encoded data
extern const char *data_encoded;
extern const char *article_0;
extern const char *article_1;
extern const char *article_2;

// Size of the decoded data
extern const size_t data_size;
extern void push_base64_to_storage(const char *b64data, const char *storage_key);
#endif // DATA_CODEC_H

