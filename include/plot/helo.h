#pragma once

// plot routine "helo" for helicopters, see for instance res/hind.ai.

// TODO: more generic version that wraps all the insane transform stuff?

// TODO: decide whether to pass entity/movement whatever as well,
// so we can decide whether or not the main rotor is static!
static inline void
sx_plot_helo(uint32_t ei)
{
  sx_entity_t *ent = sx.world.entity + ei;
  uint32_t oi = sx.world.entity[ei].objectid;
  sx_object_t *obj = sx.assets.object + oi;
  int geo_beg = obj->geo_g + obj->geo_h;
  int geo_end = MAX(geo_beg+1, obj->geo_g + obj->geo_m);

  // offsets matched in blender, needed to flip sign
  static const float part_pos[][3] =
  {
    {0, 0, 0},              // bod:  fuselage
    {0, 0, 0},              // bl1:  static main rotor
    {0, 0, 0},              // bl2:  moving main rotor
    {1.85, -1.041, -36.73}, // rt1:  static tail rotor
    {1.85, -1.041, -36.73}, // rt2:  moving tail rotor
  };
  static const float part_rot[][4] =
  {
    {    0, 0, 1, 0}, // bod  0
    {-.001, 0, 1, 0}, // bl1  1
    {-.003, 0, 1, 0}, // bl2  2
    {-.001, 1, 0, 0}, // rt1  3
    {-.003, 1, 0, 0}, // rt2  4
  };

  const float anim = sx.time;
       
  const quat_t *bq = &ent->body.q;
  for(int gg=geo_beg;gg<geo_end;gg++)
  {
    int g = gg - geo_beg;
    if(g == 1 || g == 3) continue; // skip static rotors

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

    sx_vid_push_geo_instance(obj->geoid[g], mp, &mq, mp, &mq);
  }
}
