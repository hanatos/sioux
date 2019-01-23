#ifndef __DPK4__
#define __DPK4__

#include <stdio.h>
#include "restable.h"

bool_t is_dpk4(restable_t *rt);

#define DPK4_SUBDIRS TRUE
#define DPK4_MERGE TRUE
#define DPK4_META FALSE
#define DPK4_TIME FALSE
#define DPK4_PAGE 1

bool_t dpk4_read_dir(restable_t *rt);
bool_t dpk4_fill_filename(resentry_t *re);
bool_t dpk4_extract_resource(restable_t *rt, uint32_t i);

#define dpk4_save_meta rt_not_save_meta
#define dpk4_load_meta rt_not_load_meta

bool_t dpk4_fill_name(resentry_t *re);
bool_t dpk4_prepare_dir(restable_t *rt);
bool_t dpk4_add_resource(restable_t *rt, uint32_t i);
bool_t dpk4_write_dir(restable_t *rt);

#endif
