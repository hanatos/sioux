#pragma once
#include <stdint.h>

typedef enum sx_image_format_t
{
  SX_RGBA8,
  SX_RGBA16,
  SX_RGBA32F,
}
sx_image_format_t;

typedef struct sx_image_t
{
  char filename[256];
  uint32_t wd, ht;
  sx_image_format_t format;
  void *px;
}
sx_image_t;
