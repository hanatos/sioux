#include "plot/common.h"

// plot routine "tire" for vehicles with tires, see for instance res/humvee.ai.
void
sx_plot_tire(uint32_t ei)
{
  const sx_plot_part_t part[] = {
    {0, 2, sx.time, 0, {    0, 1, 0, 0}}, // body
    {1, 2, sx.time, 0, {    0, 1, 0, 0}}, // shadow
    {2, 0, sx.time, 0, {    0, 1, 0, 0}}, // front wheel
    {2, 1, sx.time, 0, {    0, 1, 0, 0}}, // other front wheel (do these turn?)
  };
  sx_plot_parts(ei, sx.world.entity[ei].objectid, sizeof(part)/sizeof(part[0]), part, 0);
}

int
sx_plot_tire_collide(const sx_entity_t *e, sx_obb_t *box, sx_part_type_t *pt)
{
  return 0;
}
