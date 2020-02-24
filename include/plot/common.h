#pragma once

// convenience function that can be shared by many plot routines

static inline void
sx_plot_common(
    const uint32_t ei,             // entity id
    const uint32_t num_parts,      // number of parts P (elements in instances array)
    const uint32_t instances[],    // can be null, or a count per part P, sum(instances) = N, and instances[i]==0 is counted as 1 (so stuff can be switched off temporarily)
    const float    anim_time[],    // can be null, or else anim time per instance N
    const float    part_pos[][3],  // position of the instances N
    const float    part_rot[][4])  // rotation data (angle + axis) for the instances N
{
  sx_entity_t *ent = sx.world.entity + ei;
  uint32_t oi = sx.world.entity[ei].objectid;
  sx_object_t *obj = sx.assets.object + oi;
  int geo_beg = obj->geo_g + obj->geo_h;
  int geo_end = MAX(geo_beg+1, obj->geo_g + obj->geo_m);


  const quat_t *bq = &ent->body.q;
  int g = 0;
  for(int gg=0;gg<MIN(num_parts, geo_end-geo_beg);gg++)
  {
    int inst = 1;
    if(instances) inst = instances[g];
    int next_g = g + MAX(1,inst); // need to supply data for 0 instances, too
    for(int i=0;i<inst;i++,g++)
    {
      float anim = sx.time;
      if(anim_time) anim = anim_time[g];
      quat_t rq;
      if(part_rot[g][0] != 0.0f)
        quat_init_angle(&rq, anim * part_rot[g][0], part_rot[g][1], part_rot[g][2], part_rot[g][3]);
      else quat_init(&rq, 1, 0, 0, 0);
      float vo[3] = {
        ft2m(part_pos[g][0]),
        ft2m(part_pos[g][1]),
        ft2m(part_pos[g][2]),
      }; // offset in model space
      quat_transform(bq, vo); // model to world space
      float mp[3] = {
        ent->body.c[0]+vo[0],
        ent->body.c[1]+vo[1],
        ent->body.c[2]+vo[2]};
      quat_t mq;
      quat_mul(bq, &rq, &mq);

      // for motion vectors, previous frame:
      // TODO: old_anim
      const quat_t *omq = &ent->prev_q;
      quat_t orq;
      if(part_rot[g][0] != 0.0f)
        quat_init_angle(&orq, sx.time * part_rot[g][0], part_rot[g][1], part_rot[g][2], part_rot[g][3]);
      else quat_init(&orq, 1, 0, 0, 0);
      float ovo[3] = {
        ft2m(part_pos[g][0]),
        ft2m(part_pos[g][1]),
        ft2m(part_pos[g][2]),
      }; // old offset in model space is the same
      quat_transform(omq, ovo); // model to world space
      float omvx[3] = {
        ent->prev_x[0]-sx.cam.prev_x[0]+ovo[0],
        ent->prev_x[1]-sx.cam.prev_x[1]+ovo[1],
        ent->prev_x[2]-sx.cam.prev_x[2]+ovo[2]};
      quat_t iovq = sx.cam.prev_q;
      quat_conj(&iovq);
      quat_transform(&iovq, omvx);
      quat_t otmp, omvq;
      quat_mul(&iovq, omq, &otmp);
      quat_mul(&otmp, &orq, &omvq);

      sx_vid_push_geo_instance(obj->geoid[gg], mp, &mq, mp, &mq);
    }
    g = next_g;
  }
}


