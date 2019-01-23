#ifndef __PACK2__
#define __PACK2__

#include <stdio.h>
#include "restable.h"

bool_t is_pack2(restable_t *rt);

#define PACK2_SUBDIRS TRUE
#define PACK2_MERGE TRUE
#define PACK2_META FALSE
#define PACK2_TIME FALSE
#define PACK2_PAGE 1

bool_t pack2_read_dir(restable_t *rt);
bool_t pack2_fill_filename(resentry_t *re);
bool_t pack2_extract_resource(restable_t *rt, uint32_t i);

#define pack2_save_meta rt_not_save_meta
#define pack2_load_meta rt_not_load_meta

bool_t pack2_fill_name(resentry_t *re);
bool_t pack2_prepare_dir(restable_t *rt);
bool_t pack2_add_resource(restable_t *rt, uint32_t i);
bool_t pack2_write_dir(restable_t *rt);

#endif
