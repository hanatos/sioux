#include "file.h"
#include "decompress.h"
#include "pngio.h"
#include "pcxread.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <libgen.h>

typedef struct c3_pcx_t
{
  char lzp1[4]; // LZP1
  uint32_t wd;
  uint32_t ht;
  uint8_t pal[256][3]; // rgb palette?
  char CerP[4]; // CerP
  // 0x310
  uint8_t pal2[256][4]; // might be the missing alpha values??? definitely the first byte has something, but the second also low values at times (single digit)
  // 0x710 ??
  uint8_t data[];
}
c3_pcx_t;

int main(int argc, char *argv[])
{
  if(argc < 3)
  {
    fprintf(stderr, "[pcx] usage: ./pcx <input.pcx> <output.png>\n");
    exit(1);
  }

  char *fn = basename(argv[1]);
  int alphahack = 1;
  // alphahack == 1: palette entry 0 is translucent
  // alphahack == 2: continuous alpha for rotor

  // super hacky way to get smooth alpha into
  // the rotor blade texture:
  if(!strcmp(fn, "com13.pcx"))  alphahack = 2;
  if(!strcmp(fn, "com06a.pcx")) alphahack = 2;
  if(!strcmp(fn, "com06b.pcx")) alphahack = 2;

  // if map name, i.e. "c[0-9]m[0-9]*.pcx"
  // don't do alpha channel.
  if(fn[0] == 'c' && fn[2] == 'm' &&
     fn[1] <= 57  && fn[1] >= 48  &&
     fn[3] <= 57  && fn[3] >= 48)
  {
    alphahack = 0;
    if(fn[4] == 'c') alphahack = 1; // colour details have alpha
  }
  // also protect multiplayer maps (mp[0-9][0-9]*.pcx) from alpha hacks:
  if(fn[0] == 'm' && fn[1] == 'p' && 
     fn[2] <= 57  && fn[2] >= 48  &&
     fn[3] <= 57  && fn[3] >= 48)
    alphahack = 0;

  // rather be explicit on cmd line
  // char outbuf[256];
  // strncpy(outbuf, argv[0], sizeof(outbuf));
  // char *c = outbuf + strlen(outbuf);
  // while(*c != '.' && c > outbuf) c--;
  // *c++ = '.'; *c++ = 'p'; *c++ = 'n'; *c++ = 'g'; *c++ = 0;

  const char *outname = argv[2];

	uint64_t len;
	c3_pcx_t *pcx = file_load(argv[1], &len);
  // const char *magic = " Shadow Tools";
  // if(!strncmp((char *)pcx->pal[0], magic, strlen(magic)))
  if(strncmp(pcx->lzp1, "LZP1", 4))
  {
    // loads in gimp, fails in imagemagick. stole code from gimp:
    uint8_t *px = 0;
    uint32_t wd, ht;
    pcx_load(argv[1], &wd, &ht, &px, alphahack);
    png_write(outname, wd, ht, px, 8);
    free(px);
    exit(0);
  }

#if 0
  // write palettes:
  {
  FILE* f = fopen("palette.ppm", "wb");
  fprintf(f, "P6\n16 16\n255\n");
  for(int k=0;k<256;k++)
    fwrite(pcx->pal[k], 1, 3, f);
  fclose(f);
  }
  {
  FILE* f = fopen("palette2.ppm", "wb");
  fprintf(f, "P6\n16 16\n255\n");
  for(int k=0;k<256;k++)
    fwrite(pcx->pal2[k], 1, 3, f);
  fclose(f);
  }
#endif

#if 0
  int max = 0;
  for(int k=0;k<256;k++)
    if(pcx->pal2[k][1] > max) max = pcx->pal2[k][1];
  fprintf(stderr, "max [1]: %d\n", max);
#endif
  fprintf(stderr, "[pcx] %u %u\n", pcx->wd, pcx->ht);

  uint8_t *dec = pcx->data;
  uint8_t *end = (uint8_t *)pcx + len;
	
	uint8_t *comp = pcx->data;
  uint64_t comp_len = (uint8_t *)pcx + len - pcx->data;
  uint64_t dec_len = pcx->wd*pcx->ht + 768 + 12 + 6 + 300;
	dec = malloc(dec_len);
  dec_len = decompress(comp, comp_len, dec, dec_len);
  fprintf(stderr, "len comp/dec = %lu -> %lu B\n", comp_len, dec_len);
  end = dec + pcx->wd * pcx->ht;

  // look for endmarker "CerP" after palette
  int off = 0;
  for(;off<20;off++)
    if(!strncmp((char*)dec + pcx->wd*pcx->ht + 768 + off, "CerP", 4)) break;
  if(off == 20) fprintf(stderr, "[pcx] ERR: did not find palette!\n");
  // off -= 2;
  fprintf(stderr, "off = %d\n", off);

  // as it turns out often very much the same as pcx->pal except
  // for the first and last entries
  uint8_t (*pal3)[3] = (uint8_t (*)[3]) (dec + pcx->wd*pcx->ht + off);
  // space in between pixels and palette should be "LZP1" uint32_t width
  // uint32_t height sometimes with an extra zero byte
  if(strncmp((char*)dec + pcx->wd*pcx->ht, "LZP1", 4))
  {
    pal3 = pcx->pal;
    fprintf(stderr, "[pcx] ERR: did not find LZP1 end marker where we expected it!\n");
    for(int k=0;k<400;k++)
    {
      if(!strncmp((char*)dec + pcx->wd*pcx->ht + k, "LZP1", 4))
      {
        fprintf(stderr, "[pcx] XXX found it instead %d bytes after expected end\n", k);
        break;
      }
    }
#if 1
    fprintf(stderr, "got extra bytes:\n");
    for(int k=0;k<off;k++)
      fprintf(stderr, "%X ", dec[pcx->wd*pcx->ht + k]);//pal3[256][k]);
      // fprintf(stderr, "%X ", pal3[256][k]);
    fprintf(stderr, "\n");
    // for(int k=0;k<off;k++)
      // fprintf(stderr, "%1c ", dec[pcx->wd*pcx->ht + k]);//pal3[256][k]);
      // fprintf(stderr, "%1c ", pal3[256][k]);
    // fprintf(stderr, "\n");
#endif
  }
#if 0
  {
  FILE* f = fopen("palette3.ppm", "wb");
  fprintf(f, "P6\n16 16\n255\n");
  for(int k=0;k<256;k++)
    fwrite(pal3[k], 1, 3, f);
  fclose(f);
  }
#endif
#if 0
  uint8_t *pal4 = pal3[256]+6;
  {
    fprintf(stderr, "got an extra %lu bytes for pal4\n", dec_len - pcx->wd*pcx->ht - off - 768 - 6);
  FILE* f = fopen("palette4.ppm", "wb");
  fprintf(f, "P6\n16 16\n255\n");
  for(int k=0;k<256;k++)
    for(int c=0;c<3;c++)
      fwrite(pal4+k, 1, 1, f);
  fclose(f);
  }
#endif

#if 0 // XXX DEBUG raw dump of uncompressed part:
  {
    FILE *f = fopen("rawdump", "wb");
    fwrite(dec, dec_len, 1, f);
    fclose(f);
  }
#endif

  uint8_t *px = malloc(sizeof(uint8_t)*pcx->wd*pcx->ht*4);
  for(int k=0;k<pcx->wd*pcx->ht;k++)
  {
    int c = dec[k];
    px[4*k+0] = pal3[c][0];
    px[4*k+1] = pal3[c][1];
    px[4*k+2] = pal3[c][2];
    px[4*k+3] = 255;
    // if(px[4*k+0] == 0 && px[4*k+1] == 0 && px[4*k+2] == 0)
    // alpha hack:
    if(alphahack == 1)
      if(c == 0) px[4*k+3] = 0;
    // px[4*k+3] = pal4[c];
    if(alphahack == 2)
      px[4*k+3] = c <= 127 ? 255-2*c : 0;
  }
  png_write(outname, pcx->wd, pcx->ht, px, 8);
  free(px);
  free(pcx);
  if(dec != pcx->data) free(dec);
	
	exit(0);
}

