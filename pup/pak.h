#ifndef __PAK__
#define __PAK__

#include <stdio.h>
#include "restable.h"

bool_t is_pak(restable_t *rt);

#define PAK_SUBDIRS FALSE
#define PAK_MERGE FALSE
#define PAK_META FALSE
#define PAK_TIME FALSE
#define PAK_PAGE 1

bool_t pak_read_dir(restable_t *rt);
bool_t pak_fill_filename(resentry_t *re);
#define pak_extract_resource rt_extract_resource

#define pak_save_meta rt_not_save_meta
#define pak_load_meta rt_not_load_meta

bool_t pak_fill_name(resentry_t *re);
bool_t pak_prepare_dir(restable_t *rt);
#define pak_add_resource rt_add_resource
bool_t pak_write_dir(restable_t *rt);

#endif
