#pragma once

// stolen from gimp and under GPLv3

typedef struct gimp_pcx_header_t
{
  uint8_t  manufacturer;
  uint8_t  version;
  uint8_t  compression;
  uint8_t  bpp;
  uint16_t x1, y1;
  uint16_t x2, y2;
  uint16_t hdpi;
  uint16_t vdpi;
  uint8_t  colormap[48];
  uint8_t  reserved;
  uint8_t  planes;
  uint16_t bytesperline;
  uint16_t color;
  uint8_t  filler[58];
  uint8_t  data[];
}
gimp_pcx_header_t;

static inline uint64_t
readline(
    gimp_pcx_header_t *h,
    uint64_t           start,
    uint8_t           *buf,
    int                bytes)
{
  uint8_t count = 0, value = 0;

  if(h->compression)
  {
    while (bytes--)
    {
      if (count == 0)
      {
        value = h->data[start++];
        if (value < 0xc0)
        {
          count = 1;
        }
        else
        {
          count = value - 0xc0;
          value = h->data[start++];
        }
      }
      count--;
      *(buf++) = value;
    }
    return start;
  }
  else memcpy(buf, h->data+start, bytes);
  return start + bytes;
}

static inline void
load_8 (gimp_pcx_header_t *h,
        int                width,
        int                height,
        uint8_t           *buf,
        int                bytes)
{
  int    row;
  uint8_t *line = malloc(sizeof(uint8_t)*bytes);
  uint64_t pos = 0;

  for (row = 0; row < height; buf += width, ++row)
  {
    pos = readline (h, pos, line, bytes);
    memcpy (buf, line, width);
  }
  free (line);
}

static inline void
load_24 (gimp_pcx_header_t *h,
         int                width,
         int                height,
         uint8_t           *buf,
         uint16_t           bytes)
{
  int    x, y, c;
  uint8_t *line = malloc(sizeof(uint8_t)*bytes);
  uint64_t pos = 0;

  for (y = 0; y < height; buf += width * 3, ++y)
  {
    for (c = 0; c < 3; ++c)
    {
      pos = readline (h, pos, line, bytes);
      for (x = 0; x < width; ++x)
        buf[x * 4 + c] = line[x];
    }
    buf[4*x+3] = 255;
  }
  free (line);
}

static inline int
pcx_load(
    const char  *filename,
    uint32_t    *wd,
    uint32_t    *ht,
    uint8_t    **px,
    int          alphahack)
{
  uint64_t filesize = 0;
  gimp_pcx_header_t *h = file_load(filename, &filesize);

  if(!strcmp(filename, "com06a.pcx")) alphahack = 2;
  if(!strcmp(filename, "res/com06a.pcx")) alphahack = 2;

  *px = 0;

  if (h->manufacturer != 10)
  {
    fprintf(stderr, "[gimp pcx] not a pcx file!\n");
    free(h);
    return 1;
  }

  uint32_t width        = h->x2 - h->x1 + 1;
  uint32_t height       = h->y2 - h->y1 + 1;
  *wd = width;
  *ht = height;

  fprintf(stderr, "[gimp pcx] %u x %u with %u bpp and %u planes\n", width, height, h->bpp, h->planes);

  if (h->bytesperline < ((width * h->bpp + 7) / 8))
  {
    fprintf(stderr, "invalid number of bytes per line in pcx header!\n");
    free(h);
    return 2;
  }

  uint8_t *dest = malloc(4*width*height);
  *px = dest;

  if (h->bpp == 8 && h->planes == 1)
  {
    fprintf(stderr, "[gimp pcx] 8-bit indexed pcx\n");
    uint8_t *dest8 = malloc(sizeof(uint8_t)*width*height);
    load_8 (h, width, height, dest8, h->bytesperline);
    uint8_t (*cmap)[3] = (uint8_t (*)[3])((uint8_t *)h + filesize - 768);
    for(int k=0;k<width*height;k++)
    {
      dest[4*k+0] = cmap[dest8[k]][0];
      dest[4*k+1] = cmap[dest8[k]][1];
      dest[4*k+2] = cmap[dest8[k]][2];
      dest[4*k+3] = 255;
      // TODO: for com06a.pcx this fails
      // alpha hack:
      if(alphahack == 1)
      {
        if(dest8[k] == 0) dest[4*k+3] = 0;
      }
      else if(alphahack == 2)
      {
        dest[4*k+3] = dest[4*k+0] <= 127 ? 255-2*dest[4*k+0] : 0;
      }
    }
    free(dest8);
  }
  else if (h->bpp == 8 && h->planes == 3)
  {
    fprintf(stderr, "[gimp pcx] b/w pcx\n");
    load_24(h, width, height, dest, h->bytesperline);
  }
  else
  {
    fprintf(stderr, "[gimp pcx] sorry can't read this format\n");
  }
   
  free(h);
  return 0;
}
