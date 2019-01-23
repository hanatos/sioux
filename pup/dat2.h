#ifndef __DAT2__
#define __DAT2__

#include <stdio.h>
#include "restable.h"

bool_t is_dat2(restable_t *rt);

#define DAT2_SUBDIRS TRUE
#define DAT2_MERGE TRUE
#define DAT2_META FALSE
#define DAT2_TIME FALSE
#define DAT2_PAGE 1

bool_t dat2_read_dir(restable_t *rt);
bool_t dat2_fill_filename(resentry_t *re);
bool_t dat2_extract_resource(restable_t *rt, uint32_t i);

#define dat2_save_meta rt_not_save_meta
#define dat2_load_meta rt_not_load_meta

bool_t dat2_fill_name(resentry_t *re);
bool_t dat2_prepare_dir(restable_t *rt);
bool_t dat2_add_resource(restable_t *rt, uint32_t i);
bool_t dat2_write_dir(restable_t *rt);

#endif
