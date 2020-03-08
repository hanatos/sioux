#pragma once

#include "sx.h"
#include "world.h"
#include "physics/obb_obb.h"

// auto-generate all headers:
#define PLOT_LIST \
PLOT_DO(1prt)\
PLOT_DO(coma)\
PLOT_DO(glue)\
PLOT_DO(helo)\
PLOT_DO(tire)\
PLOT_DO(wolf)

#define PLOT_DO(A)\
void sx_plot_ ## A (uint32_t ei);\
int  sx_plot_ ## A ## _collide(const sx_entity_t *e, sx_obb_t *box, sx_part_type_t *pt);

PLOT_LIST
#undef PLOT_DO

static inline void
sx_plot_init(sx_entity_t *e)
{
  const char *draw = sx.assets.object[e->objectid].draw;
#define PLOT_DO(A)\
  if(!strncmp(draw, #A, 4)) {\
    e->plot = (sx_plot_t){\
      .id      = #A,\
      .plot    = sx_plot_ ## A ,\
      .collide = sx_plot_ ## A ## _collide,\
    };\
    return;\
  }
  PLOT_LIST
#undef PLOT_DO
  if(!strncmp(draw, "edit", 4))
  {
    e->plot = (sx_plot_t){
      .id = "edit",
    };
    return;
  }
  e->plot = (sx_plot_t){
    .id = "none",
    .plot = sx_plot_1prt,
  };
}
#undef PLOT_LIST

// convenience function that can be shared by many plot routines

typedef struct sx_plot_part_t
{
  int geo;      // geo index of the part
  int offset;   // offset index stored in the first 3do (or -1 for null offset)
  float time;   // animation time
  int   frame;  // animation frame if animated texture
  float rot[4]; // rotation angle + axis
}
sx_plot_part_t;

static inline void
sx_plot_parts(
    const uint32_t        ei,             // entity id
    const uint32_t        oi,             // object id, if different to entity's (dead obj)
    const uint32_t        num_parts,      // number of parts
    const sx_plot_part_t *part,
    const float          *cmm)            // center of mass in model space (or 0)
{
  sx_entity_t *ent = sx.world.entity + ei;
  sx_object_t *obj = sx.assets.object + oi;

  const quat_t *bq = &ent->body.q;
  for(int p=0;p<num_parts;p++)
  {
    const float nulloff[3] = {0};
    const float *off = part[p].offset == -1 ? nulloff : obj->geo_off[0] + 3*part[p].offset;
    int g = part[p].geo;
    float anim = part[p].time;
    quat_t rq;
    if(part[p].rot[0] != 0.0f)
      quat_init_angle(&rq, anim * part[p].rot[0], part[p].rot[1], part[p].rot[2], part[p].rot[3]);
    else quat_init(&rq, 1, 0, 0, 0);
    float vo[3] = {
      off[0], off[1], off[2]
    }; // offset in model space
    if(cmm) for(int k=0;k<3;k++) vo[k] -= cmm[k];
    quat_transform(bq, vo); // model to world space
    float mp[3] = {
      ent->body.c[0]+vo[0],
      ent->body.c[1]+vo[1],
      ent->body.c[2]+vo[2]};
    quat_t mq;
    quat_mul(bq, &rq, &mq);

#if 1 // for motion vectors, previous frame:
    // TODO: old_anim
    float ovo[3] = {
      off[0], off[1], off[2]
    }; // offset in model space
    if(cmm) for(int k=0;k<3;k++) ovo[k] -= cmm[k];
    quat_transform(&ent->prev_q, ovo); // model to world space
    float omp[3] = {
      ent->prev_x[0]+ovo[0],
      ent->prev_x[1]+ovo[1],
      ent->prev_x[2]+ovo[2]};
    quat_t omq;
    quat_mul(&ent->prev_q, &rq, &omq);
#endif
    sx_vid_push_geo_instance(obj->geoid[g], omp, &omq, mp, &mq, part[p].frame);
  }
}


