#ifndef __C_LZSS__
#define __C_LZSS__

#include <sys/types.h>
#include "bin.h"

size_t lzss_dsize(size_t ssize);
bool_t lzss_compress(unsigned char *s, size_t ssize, unsigned char *d, size_t *dsize, int level);
bool_t lzss_decompress(unsigned char *s, size_t ssize, unsigned char *d, size_t dsize);

#endif
