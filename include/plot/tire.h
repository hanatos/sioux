#pragma once
#include "plot/common.h"

// plot routine "tire" for vehicles with tires, see for instance res/humvee.ai.
static inline void
sx_plot_tire(uint32_t ei)
{
  // offsets matched in blender, needed to flip sign
  const float part_pos[][3] =
  {
    {0, 0, 0},             // h1: body
    {0, 0, 0},             // h2: drop shadow
    { 2.73, 1.667, 5.6},   // h3: front tires (need two instances)
    {-3.65, 1.667, 5.6},   // h3: front tires (need two instances)
  };
  const float part_rot[][4] =
  {
    {    0, 1, 0, 0}, // TODO: do these actually turn?
    {    0, 1, 0, 0},
    {    0, 1, 0, 0},
    {    0, 1, 0, 0},
  };
  const uint32_t inst[] =
  {
    1, 1, 2, // last part has two instances, so the above arrays hold 4 entries
  };
  sx_plot_common(ei, 3, inst, 0, part_pos, part_rot);
}
