#pragma once

typedef struct c3_jim_t
{
}
c3_jim_t;

static inline int
c3_jim_load(const char *filename)
{
  uint64_t len;
  uint8_t *dat = file_load(filename, &len);
  if(!dat) return 1;

  int i=0;
  // what's in this header?
  // uint32_t head = *(uint32_t *)dat;
  uint32_t cnt = 6;//*(uint32_t *)(dat+4);
  i = 9;
  for(int k=0;k<cnt;k++)
  {
    fprintf(stderr, "wp group %d\n", k);
    int wp = 0;
    while(i<len)
    {
      if(dat[i] == 0 && dat[i+1] == 0) { i += 2; break;}
      // int32_t x = *(uint32_t *)(dat+i);
      // i += 4;
      i+=1;
      uint16_t x0 = *(uint16_t *)(dat+i);
      i+=2;
      uint16_t x1 = *(uint16_t *)(dat+i);
      i+=2;
      // int32_t y = *(uint32_t *)(dat+i);
      // i += 4;
      uint16_t y0 = *(uint16_t *)(dat+i);
      i+=2;
      uint16_t y1 = *(uint16_t *)(dat+i);
      i+=1;
      wp++;
      // these 2*x are the same values as in .mis. great.
      fprintf(stderr, "[%d] x, y %u %u %u %u\n", wp, 2*x0, 2*x1, y0, y1);
      i++; // read null byte
      // if there's more null bytes, the names start
      if(i < len && dat[i] == 0) { i++; break; }
    }
  }
  while(i<len)
  {
    fprintf(stderr, "%s\n", (char*)dat+i);
    i+=16;
    if(i < len && dat[i] == 0) break; // final terminator byte
  }

  free(dat);
  return 0; 
}
