#pragma once
#include "file.h"
// stolen from darktable, GPLv3
#include <inttypes.h>
#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>

static inline void
png_from_pcx(const char *filename, char *fn, uint32_t fnlen)
{
  // mangle from PCX to lowercase png
  int len = strlen(filename);
  if(len > fnlen) len = fnlen;
  for(int k=0;k<len;k++)
  {
    if(filename[k] == '.')
    {
      strcpy(fn+k, ".png");
      break;
    }
    fn[k] = tolower(filename[k]);
  }
}

static inline int
png_write(
    const char *filename,
    const int width,
    const int height,
    const void *ivoid,
    const int bpp)
{
  FILE *f = fopen(filename, "wb");
  if(!f) return 1;

  png_structp png_ptr;
  png_infop info_ptr;

  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if(!png_ptr)
  {
    fclose(f);
    return 1;
  }

  info_ptr = png_create_info_struct(png_ptr);
  if(!info_ptr)
  {
    fclose(f);
    png_destroy_write_struct(&png_ptr, NULL);
    return 1;
  }

  if(setjmp(png_jmpbuf(png_ptr)))
  {
    fclose(f);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    return 1;
  }

  png_init_io(png_ptr, f);

  int compression = 1;

  png_set_compression_level(png_ptr, compression);
  png_set_compression_mem_level(png_ptr, 8);
  png_set_compression_strategy(png_ptr, Z_DEFAULT_STRATEGY);
  png_set_compression_window_bits(png_ptr, 15);
  png_set_compression_method(png_ptr, 8);
  png_set_compression_buffer_size(png_ptr, 8192);

  png_set_IHDR(png_ptr, info_ptr, width, height, bpp, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
               PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

  // metadata has to be written before the pixels
  png_write_info(png_ptr, info_ptr);

  /*
   * Get rid of filler (OR ALPHA) bytes, pack XRGB/RGBX/ARGB/RGBA into
   * RGB (4 channels -> 3 channels). The second parameter is not used.
   */
  // png_set_filler(png_ptr, 0, PNG_FILLER_AFTER);

  png_bytep *row_pointers = malloc((size_t)height * sizeof(png_bytep));

  if(bpp > 8)
  {
    /* swap bytes of 16 bit files to most significant bit first */
    png_set_swap(png_ptr);

    for(unsigned i = 0; i < height; i++) row_pointers[i] = (png_bytep)((uint16_t *)ivoid + (size_t)4 * i * width);
  }
  else
  {
    for(unsigned i = 0; i < height; i++) row_pointers[i] = (uint8_t *)ivoid + (size_t)4 * i * width;
  }

  png_write_image(png_ptr, row_pointers);

  free(row_pointers);

  png_write_end(png_ptr, info_ptr);
  png_destroy_write_struct(&png_ptr, &info_ptr);
  fclose(f);
  return 0;
}

static inline int
png_read(
    const char *filename,
    int *width,
    int *height,
    void **ovoid,
    int *bpp)
{
  FILE *f = file_open(filename);
  if(!f) return 1;

#define NUM_BYTES_CHECK (8)

  png_byte dat[NUM_BYTES_CHECK];

  size_t cnt = fread(dat, 1, NUM_BYTES_CHECK, f);

  if(cnt != NUM_BYTES_CHECK || png_sig_cmp(dat, (png_size_t)0, NUM_BYTES_CHECK))
  {
    fclose(f);
    return 1;
  }

  png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if(!png_ptr)
  {
    fclose(f);
    return 1;
  }

  png_infop info_ptr = png_create_info_struct(png_ptr);
  if(!info_ptr)
  {
    fclose(f);
    png_destroy_read_struct(&png_ptr, NULL, NULL);
    return 1;
  }

  if(setjmp(png_jmpbuf(png_ptr)))
  {
    fclose(f);
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    return 1;
  }

  png_init_io(png_ptr, f);

  // we checked some bytes
  png_set_sig_bytes(png_ptr, NUM_BYTES_CHECK);

  // image info
  png_read_info(png_ptr, info_ptr);

  uint32_t bit_depth = png_get_bit_depth(png_ptr, info_ptr);
  uint32_t color_type = png_get_color_type(png_ptr, info_ptr);

  // image input transformations

  // palette => rgb
  if(color_type == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png_ptr);

  // 1, 2, 4 bit => 8 bit
  if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) png_set_expand_gray_1_2_4_to_8(png_ptr);

  // strip alpha channel
  // if(color_type & PNG_COLOR_MASK_ALPHA) png_set_strip_alpha(png_ptr);

  // grayscale => rgb
  if(color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    png_set_gray_to_rgb(png_ptr);

  // png->bytespp = 3*bit_depth/8;
  *width  = png_get_image_width(png_ptr, info_ptr);
  *height = png_get_image_height(png_ptr, info_ptr);
  *bpp = 8;

#undef NUM_BYTES_CHECK

  if(setjmp(png_jmpbuf(png_ptr)))
  {
    fclose(f);
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    return 1;
  }

  // TODO: for 16 bits need more!
  *ovoid = malloc(sizeof(uint8_t)*4**width**height);
  png_bytep row_pointer = (png_bytep)(*ovoid);
  unsigned long rowbytes = png_get_rowbytes(png_ptr, info_ptr);

  for(int y = 0; y < *height; y++)
  {
    png_read_row(png_ptr, row_pointer, NULL);
    row_pointer += rowbytes;
  }

  png_read_end(png_ptr, info_ptr);
  png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

  fclose(f);
  return 0;
}
