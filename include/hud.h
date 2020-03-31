#pragma once
#include <stdint.h>

typedef enum sx_hud_element_t
{
  s_hud_none = 0,
  s_hud_waypoint = 4,       // name and distance to the next waypoint
  s_hud_weapons = 5,        // weapon priority indicator
  s_hud_alt_above_sea = 7,  // above sea level? (unused by us)
  s_hud_altitude = 8,       // altimeter above ground level
  s_hud_radar = 9,
  s_hud_torque = 11,
  s_hud_velocity_vec = 12,  // velocity vector (unused by us)
  s_hud_groundvel = 13,     // speed in knows ? or 7
  s_hud_rate_of_climb = 14, // rate of climb indicator
  s_hud_compass,
  s_hud_time,
  s_hud_center,
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

void sx_hud_init(sx_hud_t *hud);

void sx_hud_flash(sx_hud_t *hud, uint32_t id, uint32_t times);
