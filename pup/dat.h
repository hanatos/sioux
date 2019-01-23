#ifndef __DAT__
#define __DAT__

#include <stdio.h>
#include "restable.h"

bool_t is_dat(restable_t *rt);

#define DAT_SUBDIRS TRUE
#define DAT_MERGE TRUE
#define DAT_META FALSE
#define DAT_TIME FALSE
#define DAT_PAGE 1

bool_t dat_read_dir(restable_t *rt);
bool_t dat_fill_filename(resentry_t *re);
bool_t dat_extract_resource(restable_t *rt, uint32_t i);

#define dat_save_meta rt_not_save_meta
#define dat_load_meta rt_not_load_meta

bool_t dat_fill_name(resentry_t *re);
bool_t dat_prepare_dir(restable_t *rt);
bool_t dat_add_resource(restable_t *rt, uint32_t i);
bool_t dat_write_dir(restable_t *rt);

#endif
