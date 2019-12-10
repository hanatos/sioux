#include "bc3io.h"
#include "pngio.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
  if(argc < 3)
  {
    fprintf(stderr, "usage: png2bc3 input.png output.bc3\n");
    exit(1);
  }
  uint8_t *in;
  int bpp, wd, ht;
  int err = png_read(argv[1], &wd, &ht, (void **)&in, &bpp);
  if(err) exit(2);
  bc3_write(argv[2], wd, ht, in);
  free(in);
  exit(0);
}
