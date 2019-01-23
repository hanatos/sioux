#ifndef __C_ZLIB__
#define __C_ZLIB__

#include "bin.h"

size_t zlib_dsize(size_t ssize);
bool_t zlib_compress(unsigned char *s, size_t ssize, unsigned char *d, size_t *dsize, int level);
bool_t zlib_decompress(unsigned char *s, size_t ssize, unsigned char *d, size_t dsize);

#endif
