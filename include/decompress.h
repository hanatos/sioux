#pragma once
#include <stdlib.h>
#include <string.h>

#define STACKSIZE 1024

// cannibalised sick cousin of 'LZP1' compression.
// returns size in bytes of the decompressed output.
static inline uint64_t
decompress(
    const uint8_t  *const src,  // compressed input
    const uint64_t  src_len,    // size in bytes
    uint8_t        *const dst,  // uncompressed output
    const uint64_t  dst_len)    // max buffer size
{
  const uint8_t *esi = src;  // source pointer
  uint8_t       *edi = dst;  // destination pointer

  uint8_t       *const dst_end = dst + dst_len;
  const uint8_t *const src_end = src + src_len;

  uint32_t bit_offset = 0;
  uint32_t curr;           // currently read word
  uint32_t sp = 0;         // stack pos
  uint32_t curr_stack[STACKSIZE];

  uint32_t word0 = 0, word1 = 0, word2 = 0; // buffer the input word
  uint8_t byte0 = 0, byte1 = 0;

  // enough for 3 byte history for 13 bit hashes
  uint8_t *dict = malloc(3*8192);  // offset to start of code book
  memset(dict, 0, 3*8192);
  // starts out at 9 and is incremented until 13
  uint32_t num_bits = 9;   // determines size of dict
  uint32_t dict_ind = 258; // current dict index
  // is multiplied by 2 every time we increment num_bits, i.e. max 4x
  uint32_t dict_end = 512;

  while(edi < dst_end)
  {
    { // read num_bits
      esi += bit_offset >> 3;
      const int boffs = bit_offset & 7;
      bit_offset = num_bits + boffs;
      if(esi >= src_end) goto out;
      uint32_t word = esi[0] | (esi[1] << 8) | (esi[2] << 16) | (esi[3] << 24);
      // remove the bits we already read:
      word >>= boffs;
      word &= (1<<num_bits)-1;
      curr = word;
    }
    if((curr & 0xffff) == 0x0101) goto out;
    else if((curr & 0xffff) != 0x0100)
    {
      word0 = word2 = curr;
      if(curr >= dict_ind)
      {
        curr = word0 = word1;
        curr &= 0xffffff00;
        curr |= byte0;
        assert(sp <= STACKSIZE);
        curr_stack[sp++] = curr;
      }
      while(word0 > 0xff)
      {
        curr = dict[3*word0+2];
        assert(sp <= STACKSIZE);
        curr_stack[sp++] = curr;
        assert((dict[3*word0+0] | (dict[3*word0+1]<<8)) < (1<<num_bits)); // XXX after we're done this triggers!
        word0 = dict[3*word0+0] | (dict[3*word0+1]<<8);
      }
      // now we got (word0 < 0xff)
      curr = word0;
      byte0 = byte1 = curr & 0xff;
      assert(sp <= STACKSIZE);
      curr_stack[sp++] = curr;
      while(sp)
      {
        curr = curr_stack[--sp];
        if(edi >= dst_end)
        {
          // fprintf(stderr, "[pcx decomp] something is b0rken!\n");
          goto out; // XXX crashes without this check after file is over!
          free(dict);
          return edi - dst;
        }
        *edi++ = curr & 0xff;
      }
      dict[3*dict_ind + 0] =  word1 & 0xff;
      dict[3*dict_ind + 1] = (word1>>8) & 0xff;
      dict[3*dict_ind + 2] =  byte1;
      dict_ind++;
      word1 = word2;
      if(dict_ind < dict_end) continue; // next read iteration
      if(num_bits >= 13) continue; // bad luck, no more memory for you
      num_bits++;                  // let's increase the dict space
      dict_end *= 2;
    }
    else // start of new block
    {
      // reset dictionary, begin with small block of memory:
      num_bits = 9;
      dict_end = 512; // end
      dict_ind = 258; // current index
      { // read num_bits
        esi += bit_offset >> 3;
        const int boffs = bit_offset & 7;
        bit_offset = num_bits + boffs;
        if(esi >= src_end) break;
        uint32_t word = esi[0] | (esi[1] << 8) | (esi[2] << 16) | (esi[3] << 24);
        word >>= boffs;
        word &= (1<<num_bits)-1;
        curr = word;
      }
      word0 = word1 = curr;
      byte0 = byte1 = curr & 0xff;
      *edi = curr & 0xff;
      edi++;
    }
  }
out:
  free(dict);
  return edi - dst;
}
#undef STACKSIZE
