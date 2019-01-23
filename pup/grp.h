#ifndef __GRP__
#define __GRP__

#include <stdio.h>
#include "restable.h"

bool_t is_grp(restable_t *rt);

#define GRP_SUBDIRS FALSE
#define GRP_MERGE FALSE
#define GRP_META FALSE
#define GRP_TIME FALSE
#define GRP_PAGE 1

bool_t grp_read_dir(restable_t *rt);
bool_t grp_fill_filename(resentry_t *re);
#define grp_extract_resource rt_extract_resource

#define grp_save_meta rt_not_save_meta
#define grp_load_meta rt_not_load_meta

bool_t grp_fill_name(resentry_t *re);
bool_t grp_prepare_dir(restable_t *rt);
#define grp_add_resource rt_add_resource
bool_t grp_write_dir(restable_t *rt);

#endif
