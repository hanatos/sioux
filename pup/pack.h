#ifndef __PACK__
#define __PACK__

#include <stdio.h>
#include "restable.h"

bool_t is_pack(restable_t *rt);

#define PACK_SUBDIRS TRUE
#define PACK_MERGE TRUE
#define PACK_META FALSE
#define PACK_TIME FALSE
#define PACK_PAGE 1

bool_t pack_read_dir(restable_t *rt);
bool_t pack_fill_filename(resentry_t *re);
#define pack_extract_resource rt_extract_resource

#define pack_save_meta rt_not_save_meta
#define pack_load_meta rt_not_load_meta

bool_t pack_fill_name(resentry_t *re);
bool_t pack_prepare_dir(restable_t *rt);
bool_t pack_pack_files(restable_t *rt);
#define pack_add_resource rt_add_resource
bool_t pack_write_dir(restable_t *rt);

#endif
