#ifndef LZW
#define LZW

#include <stdint.h>
#include <stddef.h>

int lzw_compress(const unsigned char *input, size_t len,
                 uint16_t **outCodes, size_t *outCount);

unsigned char* lzw_decompress(const uint16_t *codes, size_t count,
                              size_t *outLen);

#endif
