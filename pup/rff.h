#ifndef __RFF__
#define __RFF__

#include <stdio.h>
#include "restable.h"

#define RFF_SUBDIRS FALSE
#define RFF_MERGE TRUE
#define RFF_META TRUE
#define RFF_TIME TRUE
#define RFF_PAGE 1

bool_t is_rff20(restable_t *rt);
bool_t is_rff30(restable_t *rt);
bool_t is_rff31(restable_t *rt);

bool_t rff_read_dir(restable_t *rt);
bool_t rff_fill_filename(resentry_t *re);
bool_t rff_extract_resource(restable_t *rt, uint32_t i);

#define rff_save_meta rt_save_meta
#define rff_load_meta rt_load_meta

bool_t rff_fill_name(resentry_t *re);
bool_t rff_prepare_dir(restable_t *rt);
bool_t rff_add_resource(restable_t *rt, uint32_t i);

bool_t rff20_write_dir(restable_t *rt);
bool_t rff30_write_dir(restable_t *rt);
bool_t rff31_write_dir(restable_t *rt);

#endif

