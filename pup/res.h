#ifndef __RES__
#define __RES__

#include <stdio.h>
#include "restable.h"

bool_t is_res(restable_t *rt);

#define RES_SUBDIRS FALSE
#define RES_MERGE FALSE
#define RES_META FALSE
#define RES_TIME FALSE
#define RES_PAGE 1

bool_t res_read_dir(restable_t *rt);
bool_t res_fill_filename(resentry_t *re);
#define res_extract_resource rt_extract_resource

#define res_save_meta rt_not_save_meta
#define res_load_meta rt_not_load_meta

bool_t res_fill_name(resentry_t *re);
bool_t res_prepare_dir(restable_t *rt);
bool_t res_add_resource(restable_t *rt, uint32_t i);
bool_t res_write_dir(restable_t *rt);

#endif
