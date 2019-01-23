#ifndef __C_DAT__
#define __C_DAT__

#include <sys/types.h>
#include "bin.h"

size_t dat_dsize(size_t ssize);
bool_t dat_compress(unsigned char *s, size_t ssize, unsigned char *d, size_t *dsize, int level);
bool_t dat_decompress(unsigned char *s, size_t ssize, unsigned char *d, size_t dsize);

#endif
