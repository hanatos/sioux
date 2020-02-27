#include "plot/common.h"

int
sx_plot_wolf_collide(
    const sx_entity_t *ent,
    sx_obb_t          *obb,
    sx_part_type_t    *pt)
{
  sx_obb_get(obb+0, ent, 0, 1);
  sx_obb_get(obb+1, ent, 2, 1);
  sx_obb_get(obb+2, ent, 4, 1);
  pt[0] = s_part_body;
  pt[1] = s_part_main_rotor;
  pt[2] = s_part_tail_rotor;
  return 3;
}

// plot routine "wolf" for hokums, see res/werewolf.ai.
void
sx_plot_wolf(uint32_t ei)
{
  const sx_plot_part_t dead_part[] = {
    {0, -1, sx.time, 0, {    0, 0, 1, 0}},};
  if(sx.world.entity[ei].hitpoints <= 0)
    return sx_plot_parts(ei, sx.mission.obj_dead_copt, 1, dead_part, 0);

  const sx_plot_part_t part[] = {
    {0, 1, sx.time, 0, {    0, 0, 1, 0}}, // bod  0
    {2, 1, sx.time, 0, {-.003, 0, 1, 0}}, // rt1  3
    {4, 1, sx.time, 0, { .003, 0, 1, 0}}, // rt2  4
  };
  sx_plot_parts(ei, sx.world.entity[ei].objectid, sizeof(part)/sizeof(part[0]), part, 0);
}
