#pragma once

// stolen frim gimp and under GPLv3

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
  // argh! who writes such code!
  // XXX static
    
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
  return bytes;
}

static inline void
load_8 (gimp_pcx_header_t *h,
        int                width,
        int                height,
        uint8_t           *buf,
        uint16_t           bytes)
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


#if 0
static struct {
  size_t   size;
  gpointer address;
} const pcx_header_buf_xlate[] = {
  { 1,  &pcx_header.manufacturer },
  { 1,  &pcx_header.version      },
  { 1,  &pcx_header.compression  },
  { 1,  &pcx_header.bpp          },
  { 2,  &pcx_header.x1           },
  { 2,  &pcx_header.y1           },
  { 2,  &pcx_header.x2           },
  { 2,  &pcx_header.y2           },
  { 2,  &pcx_header.hdpi         },
  { 2,  &pcx_header.vdpi         },
  { 48, &pcx_header.colormap     },
  { 1,  &pcx_header.reserved     },
  { 1,  &pcx_header.planes       },
  { 2,  &pcx_header.bytesperline },
  { 2,  &pcx_header.color        },
  { 58, &pcx_header.filler       },
  { 0,  NULL }
};

static void
pcx_header_from_buffer (guint8 *buf)
{
  gint i;
  gint buf_offset = 0;

  for (i = 0; pcx_header_buf_xlate[i].size != 0; i++)
    {
      memmove (pcx_header_buf_xlate[i].address, buf + buf_offset,
               pcx_header_buf_xlate[i].size);
      buf_offset += pcx_header_buf_xlate[i].size;
    }
}
#endif

static inline int
pcx_load(
    const char  *filename,
    uint32_t    *wd,
    uint32_t    *ht,
    uint8_t    **px,
    int          alphahack)
{
  // uint8_t  cmap[768];

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

#if 0
  // XXX i think this is the only case i care about:
  if (pcx_header.planes == 3 && pcx_header.bpp == 8)
    {
      image= gimp_image_new (width, height, GIMP_RGB);
      layer= gimp_layer_new (image, _("Background"), width, height,
                             GIMP_RGB_IMAGE,
                             100,
                             gimp_image_get_default_new_layer_mode (image));
    }
  else
    {
      image= gimp_image_new (width, height, GIMP_INDEXED);
      layer= gimp_layer_new (image, _("Background"), width, height,
                             GIMP_INDEXED_IMAGE,
                             100,
                             gimp_image_get_default_new_layer_mode (image));
    }
#endif

#if 0
  if (pcx_header.planes == 1 && pcx_header.bpp == 1)
    {
      dest = g_new (guchar, ((gsize) width) * height);
      load_1 (fd, width, height, dest, bytesperline);
      /* Monochrome does not mean necessarily B&W. Therefore we still
       * want to check the header palette, even for just 2 colors.
       * Hopefully the header palette will always be filled with
       * meaningful colors and the creator software did not just assume
       * B&W by being monochrome.
       * Until now test samples showed that even when B&W the header
       * palette was correctly filled with these 2 colors and we didn't
       * find counter-examples.
       * See bug 159947, comment 21 and 23.
       */
      gimp_image_set_colormap (image, pcx_header.colormap, 2);
    }
  else if (pcx_header.bpp == 1 && pcx_header.planes == 2)
    {
      dest = g_new (guchar, ((gsize) width) * height);
      load_sub_8 (fd, width, height, 1, 2, dest, bytesperline);
      gimp_image_set_colormap (image, pcx_header.colormap, 4);
    }
  else if (pcx_header.bpp == 2 && pcx_header.planes == 1)
    {
      dest = g_new (guchar, ((gsize) width) * height);
      load_sub_8 (fd, width, height, 2, 1, dest, bytesperline);
      gimp_image_set_colormap (image, pcx_header.colormap, 4);
    }
  else if (pcx_header.bpp == 1 && pcx_header.planes == 3)
    {
      dest = g_new (guchar, ((gsize) width) * height);
      load_sub_8 (fd, width, height, 1, 3, dest, bytesperline);
      gimp_image_set_colormap (image, pcx_header.colormap, 8);
    }
  else if (pcx_header.bpp == 1 && pcx_header.planes == 4)
    {
      dest = g_new (guchar, ((gsize) width) * height);
      load_4 (fd, width, height, dest, bytesperline);
      gimp_image_set_colormap (image, pcx_header.colormap, 16);
    }
  else if (pcx_header.bpp == 4 && pcx_header.planes == 1)
    {
      dest = g_new (guchar, ((gsize) width) * height);
      load_sub_8 (fd, width, height, 4, 1, dest, bytesperline);
      gimp_image_set_colormap (image, pcx_header.colormap, 16);
    }
#endif
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
#if 0
  else
    {
      g_message (_("Unusual PCX flavour, giving up"));
      fclose (fd);
      return -1;
    }
#endif
   
  free(h);
  return 0;
}

#if 0

static inline void
load_1 (FILE    *fp,
        int      width,
        int      height,
        uint8_t *buf,
        uint16_t bytes)
{
  int    x, y;
  uint8_t *line = malloc(sizeof(uint8_t)*bytes);

  for (y = 0; y < height; buf += width, ++y)
  {
    readline (fp, line, bytes);
    for (x = 0; x < width; ++x)
    {
      if (line[x / 8] & (128 >> (x % 8)))
        buf[x] = 1;
      else
        buf[x] = 0;
    }
  }
  free (line);
}

static inline void
load_4 (FILE    *fp,
        int      width,
        int      height,
        uint8_t *buf,
        uint16_t bytes)
{
  int    x, y, c;
  uint8_t *line = malloc(sizeof(uint8_t)*bytes);

  for (y = 0; y < height; buf += width, ++y)
  {
    for (x = 0; x < width; ++x)
      buf[x] = 0;
    for (c = 0; c < 4; ++c)
    {
      readline(fp, line, bytes);
      for (x = 0; x < width; ++x)
      {
        if (line[x / 8] & (128 >> (x % 8)))
          buf[x] += (1 << c);
      }
    }
  }
  free (line);
}

static inline void
load_sub_8 (FILE     *fp,
            int       width,
            int       height,
            int       bpp,
            int       plane,
            uint8_t  *buf,
            uint16_t  bytes)
{
  int      x, y, c, b;
  uint8_t *line = g_new (guchar, bytes);
  int      real_bpp = bpp - 1;
  int      current_bit = 0;

  for (y = 0; y < height; buf += width, ++y)
  {
    for (x = 0; x < width; ++x)
      buf[x] = 0;
    for (c = 0; c < plane; ++c)
    {
      readline (fp, line, bytes);
      for (x = 0; x < width; ++x)
      {
        for (b = 0; b < bpp; b++)
        {
          current_bit = bpp * x + b;
          if (line[current_bit / 8] & (128 >> (current_bit % 8)))
            buf[x] += (1 << (real_bpp - b + c));
        }
      }
    }
  }
  free (line);
}

#endif

