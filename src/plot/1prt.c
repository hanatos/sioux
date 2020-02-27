#include "plot/common.h"

int
sx_plot_1prt_collide(const sx_entity_t *e, sx_obb_t *box, sx_part_type_t *pt)
{
  return 0;
}

// in fact this works for any number of parts, it just plots them without
// animation or offsets whatsoever.
void
sx_plot_1prt(uint32_t ei)
{
  sx_entity_t *e = sx.world.entity + ei;
  int oi = e->objectid;

  const quat_t *mq  = &e->body.q;
  const quat_t *omq = &e->prev_q;
  const float  *mp  =  e->body.c;
  const float  *omp =  e->prev_x;

  sx_object_t *obj = sx.assets.object + oi;

  // TODO: determine LOD h m l?
  // const int lod = 0;
  // TODO: desert snow or green?
  int geo_beg = obj->geo_g + obj->geo_h;
  int geo_end = MAX(geo_beg+1, obj->geo_g + obj->geo_m);

  for(int g=geo_beg;g<geo_end;g++)
    sx_vid_push_geo_instance(obj->geoid[g], omp, omq, mp, mq, (sx.time-e->birth_time)/200);
}

