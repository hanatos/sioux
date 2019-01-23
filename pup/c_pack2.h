#ifndef __C_PACK2__
#define __C_PACK2__

#include <sys/types.h>
#include "bin.h"

size_t pack2_dsize(size_t ssize);
bool_t pack2_compress(unsigned char *s, size_t ssize, unsigned char *d, size_t *dsize, int level);
bool_t pack2_decompress(unsigned char *s, size_t ssize, unsigned char *d, size_t dsize);

#endif
