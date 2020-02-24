#pragma once

#include "plot/common.h"

// plot routine "helo" for helicopters, see for instance res/hind.ai.
static inline void
sx_plot_helo(uint32_t ei)
{
  // offsets matched in blender, needed to flip sign
  const float part_pos[][3] =
  {
    {0, 0, 0},              // bod:  fuselage
    {0, 0, 0},              // bl1:  static main rotor
    {0, 0, 0},              // bl2:  moving main rotor
    {1.85, -1.041, -36.73}, // rt1:  static tail rotor
    {1.85, -1.041, -36.73}, // rt2:  moving tail rotor
  };
  const float part_rot[][4] =
  {
    {    0, 0, 1, 0}, // bod  0
    {-.001, 0, 1, 0}, // bl1  1
    {-.003, 0, 1, 0}, // bl2  2
    {-.001, 1, 0, 0}, // rt1  3
    {-.003, 1, 0, 0}, // rt2  4
  };
  const uint32_t inst[] =
  {
    1, 0, 1, 0, 1, // switch off static rotors
  };
  sx_plot_common(ei, 5, inst, 0, part_pos, part_rot);
}
