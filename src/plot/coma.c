#include "util.h"
#include "plot/common.h"
#include "physics/obb_obb.h"

// plot the hero comanche asset

// model is in x left, y back, z top space

// list of parts, from player.ai (there are variants of this list cm[abc]h)
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

// deadcoma.ai: (x0)
// deadefam.ai: (x)
// com[abc]x[0]_h.3do // dead version with (x0) or without (x) extra rockets at the side
// fire.ai:
// flames.3do         // fire inside dead comanche


// animation data:
// these seem to be 1D animation curves, some for rotation. we might possibly do better without them.
// door.kda   // flaps open?
// rotor.kda  // rear rotor?
// gear.kda   // retract/expand gear?
// thunk.ka   // hit obstacle/terrain

// animations:
// bl1, bl2: main rotor rotates around z (up)
// rtr: tail rotor rotates around x (right)

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

int
sx_plot_coma_collide(
    const sx_entity_t *ent,
    sx_obb_t          *obb,
    sx_part_type_t    *pt)
{
  sx_obb_get(obb+0, ent, 0, -1);
  sx_obb_get(obb+1, ent, 2, -1);
  sx_obb_get(obb+2, ent, 3,  2);
  pt[0] = s_part_body;
  pt[1] = s_part_main_rotor;
  pt[2] = s_part_tail_rotor;
  return 3;
}

void
sx_plot_coma(uint32_t ei)
{
  sx_entity_t *ent = sx.world.entity + ei;
  const uint32_t oi = sx.world.entity[ei].objectid;
  const sx_plot_part_t dead_part[] = {
    { 0, -1, sx.time,     0, {    0, 0, 1, 0}},};
  if(sx.world.entity[ei].hitpoints <= 0)
    return sx_plot_parts(ei, sx.mission.obj_dead_coma, 1, dead_part, 0);

  float bay  = ent->stat.bay;
  float gear = ent->stat.gear;
  const sx_plot_part_t part[] = {
    { 0, -1, sx.time,     0, {    0, 0, 1, 0}},        // bod  0
  //{ 1, -1, sx.time,     0, { .001, 0, 1, 0}},        // bl1  1
    { 2, -1, sx.time,     0, { .003, 0, 1, 0}},        // bl2  2
    { 3,  2, sx.time,     0, { .010, 1, 0, 0}},        // rtr  3
    { 4, -1, sx.time,     0, {    0, 0, 1, 0}},        // int
    { 5,  3, gear,        0, {-1.52,  0.1, 0.1, 0.9}}, // ldr  5
    { 6,  4, gear,        0, { 1.52, -0.1, 0.1, 0.9}}, // ldl  6
    { 7,  5, gear,        0, {-1.52, 0, 0, 1}},        // ldk  7
    { 8,  6, bay,         0, {-1.40, 0, 0.1, 1}},      // mdr  8 bay door
    { 9,  7, bay,         0, { 1.40, 0, 0.1, 1}},      // mdl  9 bay door
    {10,  8, gear,        0, {-1.39, .8,-0.3, 0.1}},   // lgr 10
    {11,  9, gear,        0, { 1.39,-.8,-0.3, 0.1}},   // lgl 11
    {12, 10, gear,        0, {-1.00, 1, 0, 0}},        // lgk 12
    {13, 12, sx.time,     0, {    0, 0, 1, 0}},        // gun
    {14, 14, sx.time,     0, {    0, 0, 1, 0}},        // hed copilot head
    {15,  6, bay,         0, {-1.40, 0, 0.1, 1}},      // wpr 15 bay weapons
    {16,  7, bay,         0, { 1.40, 0, 0.1, 1}},      // wpl 16 bay weapons
    {17, 12, sx.time,     0, {    0, 0, 1, 0}},        // gn2
  //{18, -1, sx.time,     0, {    0, 0, 1, 0}},        // fam 18 extra weapons
    {19, -1, sx.time,     0, {    0, 0, 1, 0}},        // in2
    {20, 11, gear,        0, { 1.52, 0, 0, 1}},        // ldj 20
    {21, 17, sx.time,     0, { 2e-3, 0, 1, 0}},        // nos 21
    {22, 12, sx.time,     0, {    0, 0, 1, 0}},        // box gun attachment point
    {14, 13, sx.time,     0, {    0, 0, 1, 0}},        // hed pilot head
  };
  int num = sizeof(part)/sizeof(part[0]);
  // don't render our head
  if((sx.cam.mode == s_cam_inside_cockpit) ||
      (sx.cam.mode == s_cam_left) ||
      (sx.cam.mode == s_cam_right)) num--;
  sx_plot_parts(ei, oi, num, part, ent->cmm);
}
