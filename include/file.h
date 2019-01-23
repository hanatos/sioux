#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

static inline FILE*
file_open(
    const char *path)
{
  // open path, res/path, and maybe in the far future
  // look into resource.res and point the file pointer there
  FILE *f = fopen(path, "rb");
  if(f) return f;
  char path2[1024];
  snprintf(path2, sizeof(path2), "res/%s", path);
  f = fopen(path2, "rb");
  if(f) return f;
  snprintf(path2, sizeof(path2), "tmp/%s", path);
  f = fopen(path2, "rb");
  return f;
}


static inline void*
file_load(
    const char *path,  // filename to load
    uint64_t   *size)  // if !0 length of file will be stored here
{
  FILE *f = fopen(path, "rb");
  if(!f)
  {
    char path2[1024];
    snprintf(path2, sizeof(path2), "res/%s", path);
    f = fopen(path2, "rb");
  }
  if(!f)
  {
    printf("[file] could not open %s\n", path);
    return 0;
  }
  fseek(f, 0, SEEK_END);
  size_t s = ftell(f);
  rewind(f);

  if(size) *size = s;

  uint8_t *ret = malloc(s + 1);
  if(fread(ret, 1, s, f) != s) {
    printf("[file] could not read file %s\n", path);
    fclose(f);
    free(ret);
    return 0;
  }
  ret[s] = 0;
  fclose(f);

  return ret;
}

static inline int
file_readline(FILE *f, char *line)
{
  while(!feof(f))
  {
    fscanf(f, "%[^\r\n]\r\n", line);
    if(line[0] == ';') continue; // poorest parsing ever
    // ignore ; comments and force lower case
    uint64_t len = strlen(line);
    for(int k=0;k<len;k++)
    {
      if(line[k] == ';')
      {
        line[k] = 0;
        break;
      }
      else line[k] = tolower(line[k]);
    }
    return 0;
  }
  return 1;
}
