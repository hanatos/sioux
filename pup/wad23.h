#ifndef __WAD23__
#define __WAD23__

#include <stdio.h>
#include "restable.h"

bool_t is_wad2(restable_t *rt);
bool_t is_wad3(restable_t *rt);

#define WAD23_SUBDIRS FALSE
#define WAD23_MERGE TRUE
#define WAD23_META FALSE
#define WAD23_TIME FALSE
#define WAD23_PAGE 1

bool_t wad23_read_dir(restable_t *rt);
bool_t wad23_fill_filename(resentry_t *re);
#define wad23_extract_resource rt_extract_resource

#define wad23_save_meta rt_not_save_meta
#define wad23_load_meta rt_not_load_meta

bool_t wad23_fill_name(resentry_t *re);
bool_t wad23_prepare_dir(restable_t *rt);
#define wad23_add_resource rt_add_resource

bool_t wad2_write_dir(restable_t *rt);
bool_t wad3_write_dir(restable_t *rt);

#endif
