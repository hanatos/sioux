#ifndef __GOB__
#define __GOB__

#include <stdio.h>
#include "restable.h"

bool_t is_gob(restable_t *rt);

#define GOB_SUBDIRS FALSE
#define GOB_MERGE TRUE
#define GOB_META FALSE
#define GOB_TIME FALSE
#define GOB_PAGE 1

bool_t gob_read_dir(restable_t *rt);
bool_t gob_fill_filename(resentry_t *re);
#define gob_extract_resource rt_extract_resource

#define gob_save_meta rt_not_save_meta
#define gob_load_meta rt_not_load_meta

bool_t gob_fill_name(resentry_t *re);
bool_t gob_prepare_dir(restable_t *rt);
#define gob_add_resource rt_add_resource
bool_t gob_write_dir(restable_t *rt);

#endif
