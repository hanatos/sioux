#pragma once

#include "util.h"

// everything associated with the hero comanche asset

// model is in x left, y back, z top space

// list of parts, from player.ai (there are variants of this list)
// cmah_bod.3do  // main body
// cmah_bl1.3do  // blade 1, static
// cmah_bl2.3do  // blade 2, in flight (needs transparent texture)
// cmah_rtr.3do  // tail rotor, just one quad that needs texture + animation
// cmah_int.3do  // interior only visible if gear is down
// cmah_ldr.3do  // right side flap for gear
// cmah_ldl.3do  // left side flap for gear
// cmah_ldk.3do  // left side flap for rear gear (single wheel)
// cmah_mdr.3do  // right side flap for weapons
// cmah_mdl.3do  // left side flap for weapons
// cmah_lgr.3do  // gear wheel right
// cmah_lgl.3do  // gear wheel left
// cmah_lgk.3do  // gear wheel rear
// cmah_gun.3do  // front gun
// cmah_hed.3do  // pilot heads?
// cmah_wpr.3do  // right weapon under the flap
// cmah_wpl.3do  // left weapon under the flap
// cmah_gn2.3do  // front gun with muzzle flash
// cmah_fam.3do  // extra weapons on wings both sides
// cmah_in2.3do  // interior of helicopter, visible when weapon flaps are open
// cmah_ldj.3do  // right side flap for rear gear (single wheel)
// cmah_nos.3do  // radar nose on front of helicopter
// cmah_box.3do  // joint between gun and below the cockpit
// coma_l.3do    // very low detail version, let's ignore this

static const char *com_part_name[] =
{
  "bod", "bl1", "bl2", "rtr", "int", "ldr", "ldl", "ldk", "mdr", "mdl",
  "lgr", "lgl", "lgk", "gun", "hed", "wpr", "wpl", "gn2", "fam", "in2",
  "ldj", "nos", "box",
};
static const int com_part_count = sizeof(com_part_name)/sizeof(com_part_name[0]);


// animation data:
// these seem to be 1D animation curves, some for rotation. we might possibly do better without them.
// door.kda   // flaps open?
// rotor.kda  // rear rotor?
// gear.kda   // retract/expand gear?
// thunk.ka   // hit obstackle/terrain


// hardcoded offsets of all parts to match body of comanche
static const int32_t com_part_pos[][3] =
{
  {0, 0, 0},                   // bod
  {0, 0, 0},                   // bl1
  {0, 0, 0},                   // bl2
  { -15047, 1442831, -478874}, // rtr
  {0, 0, 0},                   // int
  { 138438, -387917, -462266}, // ldr
  {-138701, -387887, -461527}, // ldl
  { 52572,  1108194, -597528}, // ldk
  { 202456,    8768, -425159}, // mdr
  {-205495,    9019, -425896}, // mdl
  { 105783, -524700, -468783}, // lgr
  {-104856, -524280, -458745}, // lgl
  {   8406, 1003972, -583477}, // lgk
  {      0, -773217, -537635}, // gun
  {      0, -594014, -217542}, // hed TODO: need two of these!
//{      0, -300386, -177350}, // hed TODO: need two of these!
  { 200000,   47424, -425401}, // wpr
  {-200000,   47424, -425401}, // wpl
  {      0, -773217, -537635}, // gn2
  {0, 0, 0},                   // fam
  {0, 0, 0},                   // in2
  { -35009, 1108094, -597812}, // ldj
  {    695, -977956, -314784}, // nos
  {   1358, -756732, -522800}, // box
  {0, 0, 0},                   // coma_l
};

// TODO: animations:
// bl1, bl2: main rotor rotates around z (up)
// rtr: tail rotor rotates around x (right)
// for quaternions: angle * dt, x y z
static const float com_part_rot[][4] =
{
  {    0, 0, 1, 0}, // bod  0
  {-.001, 0, 1, 0}, // bl1  1
  {-.003, 0, 1, 0}, // bl2  2
  { .010, 1, 0, 0}, // rtr  3
  {    0, 0, 1, 0}, // int
  {-1.52,  0.1, 0.1, 0.9}, // ldr  5
  { 1.52, -0.1, 0.1, 0.9}, // ldl  6
  {-1.52, 0, 0, 1}, // ldk  7
  {-1.40, 0, 0.1, 1}, // mdr  8 bay door
  { 1.40, 0, 0.1, 1}, // mdl  9 bay door
  {-1.39, .8,-0.3, 0.1}, // lgr 10
  { 1.39,-.8,-0.3, 0.1}, // lgl 11
  {-1.00, 1, 0, 0}, // lgk 12
  {    0, 0, 1, 0}, // gun
  {    0, 0, 1, 0}, // hed TODO: need two of these!
//{    0, 0, 1, 0}, // hed TODO: need two of these!
  {-1.40, 0, 0.1, 1}, // wpr 15 bay weapons
  { 1.40, 0, 0.1, 1}, // wpl 16 bay weapons
  {    0, 0, 1, 0}, // gn2
  {    0, 0, 1, 0}, // fam 18
  {    0, 0, 1, 0}, // in2
  { 1.52, 0, 0, 1}, // ldj 20
  { 2e-3, 0, 1, 0}, // nos 21
  {    0, 0, 1, 0}, // box
  {    0, 0, 1, 0}, // coma_l
};

// raise/lower gear:
// ldr,ldl,ldk,ldj rotate around y (front) not sure it's exactly (0,1,0) though
// lgr,lgl,lgk     rotate around x (right)

// open weapons flap:
// mdl,mdr  rotate around y (front)
// wpr,wpl  rotate the same with it

// pilot head free look
// hed, hed rotate around at least z but maybe a bit more like (0,1,0.1)

// gun free look
// gun,gn2 rotate freely to direct fire to lookat point. lock with 2nd pilot head?

// TODO: also request heli_t movement params!
static inline void
sx_coma_render(uint32_t ei, sx_heli_t *h)
{
#if 1
  // XXX put into move routines!
  if(h->ctl.gear_move > 0 && h->ctl.gear < 1.0f)
    h->ctl.gear += 1.0f/128.0f;
  if(h->ctl.gear_move < 0 && h->ctl.gear > 0.0f)
    h->ctl.gear -= 1.0f/128.0f;
  if(h->ctl.gear == 0.0f) h->ctl.gear_move = 0; // not really needed i guess
  // fprintf(stderr, "gear %g\n", h->ctl.gear);
  if(h->ctl.flap_move > 0 && h->ctl.flap < 1.0f)
    h->ctl.flap += 1.0f/128.0f;
  if(h->ctl.flap_move < 0 && h->ctl.flap > 0.0f)
    h->ctl.flap -= 1.0f/128.0f;
  if(h->ctl.flap == 0.0f) h->ctl.flap_move = 0; // not really needed i guess
#endif
  sx_entity_t *ent = sx.world.entity + ei;
  uint32_t oi = sx.world.entity[ei].objectid;
  sx_object_t *obj = sx.assets.object + oi;

  // 0) apply rotation rq and model coordinate offset (center of mass to object rotation center, vo)
  // 1) transform model to world space orientation (model q)
  // 2) add world space position of model's center of mass (model x)
  // 3) subtract camera position (-view x)
  // 4) rotate world to camera   (inv view q)

  //   ivq * (-vx + mx + mq * (rq*v + vo))
  // = ivq * (mx - vx + mq * vo) + (ivq mq rq) * v
  //   `---------mvx-----------'   `---mvq---'    <= store those two (vec + quat)

  int geo_beg = obj->geo_g + obj->geo_h;
  int geo_end = MAX(geo_beg+1, obj->geo_g + obj->geo_m);

  for(int g=geo_beg;g<geo_end;g++)
  {
    if(g==1) continue; // skip static rotor
    if(g==18) continue; // no extra weapons on side
    // TODO: special treatment: if bl2 (main rotor in motion), push vertices up depending on cyclic!
    if(g == 14 &&
       ((sx.cam.mode == s_cam_inside_cockpit) ||
        (sx.cam.mode == s_cam_left) ||
        (sx.cam.mode == s_cam_right)))
      continue; // don't render our head

    float anim = sx.time;
    // if(g == 2 || g == 3) // main or tail rotor
      // anim = sx.time;
    if(g == 10 || g == 11 || g == 12) // wheels
      anim = h->ctl.gear;
    else if(g == 5 || g == 6 || g == 7 || g == 20) // bay doors for wheels
      anim = h->ctl.gear;
    else if(g == 15 || g == 16 || g == 8 || g == 9) // weapons bay doors
      anim = h->ctl.flap;
       
    const quat_t *bq = &ent->body.q;
    quat_t rq;
    if(com_part_rot[g][0] != 0.0f)
      quat_init_angle(&rq, anim * com_part_rot[g][0], com_part_rot[g][1], com_part_rot[g][2], com_part_rot[g][3]);
    else quat_init(&rq, 1, 0, 0, 0);
    float vo[3] = {
      // FIXME: can we please sanitize this mess? (see hero.vert)
      -h->cmm[0] + ft2m(-com_part_pos[g][0]/(float)0xffff),
      -h->cmm[1] + ft2m( com_part_pos[g][2]/(float)0xffff),
      -h->cmm[2] + ft2m(-com_part_pos[g][1]/(float)0xffff),
    }; // offset in model space
    quat_transform(bq, vo); // model to world space
    float mp[3] = {
      ent->body.c[0]+vo[0],
      ent->body.c[1]+vo[1],
      ent->body.c[2]+vo[2]};
    quat_t mq;
    quat_mul(bq, &rq, &mq);
#if 0
    float mvx[3] = {
      ent->body.c[0]-sx.cam.x[0]+vo[0],
      ent->body.c[1]-sx.cam.x[1]+vo[1],
      ent->body.c[2]-sx.cam.x[2]+vo[2]};
    quat_t ivq = sx.cam.q;
    quat_conj(&ivq);
    quat_transform(&ivq, mvx);
    quat_t tmp, mvq;
    quat_mul(&ivq, mq, &tmp);
    quat_mul(&tmp, &rq, &mvq);
#endif

    // for motion vectors, previous frame:
    const quat_t *omq = &ent->prev_q;
    quat_t orq;
    if(com_part_rot[g][0] != 0.0f)
      quat_init_angle(&orq, sx.time * com_part_rot[g][0], com_part_rot[g][1], com_part_rot[g][2], com_part_rot[g][3]);
    else quat_init(&orq, 1, 0, 0, 0);
    float ovo[3] = {
      // FIXME: can we please sanitize this mess?
      -h->cmm[0] + ft2m(-com_part_pos[g][0]/(float)0xffff),
      -h->cmm[1] + ft2m( com_part_pos[g][2]/(float)0xffff),
      -h->cmm[2] + ft2m(-com_part_pos[g][1]/(float)0xffff),
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

    // XXX TODO: consider transformation/rotation!
    sx_vid_render_geo(obj->geoid[g], mp, &mq, mp, &mq);
  }
}

#if 0
// debug: dump whole comanche as obj and apply offsets on the way
static inline void
com_debug_dump_obj()
{
  char filename[256];
  char outputfn[256];
  // for all parts
  // load 3do, apply offset, dump obj
  for(int i=0;i<com_part_count;i++)
  {
    snprintf(filename, sizeof(filename), "res/cmah_%s.3do", com_part_name[i]);
    snprintf(outputfn, sizeof(outputfn), "tmp/cmah_%s.obj", com_part_name[i]);
    c3m_header_t *h = file_load(filename, 0);
    if(!h) exit(1);
    for(int v=0;v<c3m_num_vertices(h);v++)
    {
      c3m_vertex_t *v0 = c3m_get_vertices(h) + v;
      for(int k=0;k<3;k++)
        v0->pos[k] += com_part_pos[i][k];
    }
    c3m_dump_obj(h, outputfn);
    free(h);
  }
}
#endif
