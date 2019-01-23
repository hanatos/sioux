#pragma once

#include "physics/heli.h"

typedef enum sx_hud_element_t
{
  s_hud_none = 0,
  s_hud_torque = 11,
  s_hud_groundvel = 13, // ? or 7
  // 14 rate of climb indicator
  s_hud_altitude = 8, // altimeter above ground level
  s_hud_compass,
  s_hud_waypoint,
  s_hud_time,
}
sx_hud_element_t;

typedef struct sx_hud_t
{
  float lines[4000];
  int num_lines;

  uint32_t flash_id;
  uint32_t flash_beg;
  uint32_t flash_end;
  uint32_t flash_text_beg;
  uint32_t flash_text_end;
  uint32_t flash_num;

  float col[3];
  float flash_col[3];
}
sx_hud_t;

void sx_hud_init(sx_hud_t *hud, const sx_heli_t *heli);

void sx_hud_flash(sx_hud_t *hud, uint32_t id, uint32_t times);
