#ifndef __VIV__
#define __VIV__

#include <stdio.h>
#include "restable.h"

bool_t is_viv(restable_t *rt);

#define VIV_SUBDIRS FALSE
#define VIV_MERGE TRUE
#define VIV_META FALSE
#define VIV_TIME FALSE
#define VIV_PAGE 4

bool_t viv_read_dir(restable_t *rt);
bool_t viv_fill_filename(resentry_t *re);
#define viv_extract_resource rt_extract_resource

#define viv_save_meta rt_not_save_meta
#define viv_load_meta rt_not_load_meta

bool_t viv_fill_name(resentry_t *re);
bool_t viv_prepare_dir(restable_t *rt);
#define viv_add_resource rt_add_resource
bool_t viv_write_dir(restable_t *rt);

#endif
