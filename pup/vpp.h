#ifndef __VPP__
#define __VPP__

#include <stdio.h>
#include "restable.h"

bool_t is_vpp(restable_t *rt);

#define VPP_SUBDIRS FALSE
#define VPP_MERGE FALSE
#define VPP_META FALSE
#define VPP_TIME FALSE
#define VPP_PAGE 2048

bool_t vpp_read_dir(restable_t *rt);
bool_t vpp_fill_filename(resentry_t *re);
#define vpp_extract_resource rt_extract_resource

#define vpp_save_meta rt_not_save_meta
#define vpp_load_meta rt_not_load_meta

bool_t vpp_fill_name(resentry_t *re);
bool_t vpp_prepare_dir(restable_t *rt);
#define vpp_add_resource rt_add_resource
bool_t vpp_write_dir(restable_t *rt);

#endif
