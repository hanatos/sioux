#pragma once
// bc1 output, stolen from vkdt (MIT licence)

#include "file.h"
#define STB_DXT_IMPLEMENTATION
#include "stb_dxt.h"
#include <stdint.h>
#include <stdio.h>

static const int bc3_magic = (('z'<<24)|('3'<<16)|('c'<<8)|'b');

static inline void
bc3_write(
    const char *filename,
    const int wd,
    const int ht,
    const uint8_t *in)
{
  const int bx = (wd+3)/4, by = (ht+3)/4;
  size_t num_blocks = bx * by;
  uint8_t *out = malloc(sizeof(uint8_t)*16*num_blocks);
  for(int j=0;j<4*by;j+=4)
  {
    for(int i=0;i<4*bx;i+=4)
    {
      // swizzle block data together:
      uint8_t block[64];
      for(int jj=0;jj<4;jj++)
        for(int ii=0;ii<4;ii++)
          for(int c=0;c<4;c++)
            block[4*(4*jj+ii)+c] = in[4*(wd*(j+jj)+(i+ii))+c];

      // with alpha enabled, this is not bc1 but bc3
      stb_compress_dxt_block(
          out + 16*(bx*(j/4)+(i/4)), block, 1 /* alpha */,
          STB_DXT_HIGHQUAL);
    }
  }
  // vkdt's files are gzipped, we just write them straight (for loading speed):
  FILE *f = fopen(filename, "wb");
  // write magic, version, width, height
  uint32_t header[4] = { bc3_magic, 1, wd, ht };
  fwrite(header, sizeof(uint32_t), 4, f);
  fwrite(out, sizeof(uint8_t)*16, num_blocks, f);
  free(out);
  fclose(f);
}

static inline int
bc3_read(
    const char *filename,
    int        *wd,
    int        *ht,
    uint8_t   **out)
{
 uint32_t header[4] = {0};
 FILE *f = file_open(filename);
 if(!f || fread(header, sizeof(uint32_t), 4, f) != 4)
 {
   if(f) fclose(f);
   return 1;
 }
 // checks: magic != "bc3z" || version != 1
 if(header[0] != bc3_magic || header[1] != 1)
 {
   fclose(f);
   return 2;
 }
 int w = header[2], h = header[3];
 size_t size = 16*((w+3)/4)*((h+3)/4);
 *wd = w; *ht = h;
 *out = malloc(sizeof(uint8_t)*size);
 fread(*out, sizeof(uint8_t), size, f);
 fclose(f);
 return 0;
}
