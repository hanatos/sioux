#pragma once
#include <stdint.h>
#include <float.h>
// parse ai file
typedef struct sx_object_t
{
  char filename[15];
  float aabb[6];          // combined bounding box of all geo
  float geo_aabb[100][6]; // aabb of individual geo parts
  uint32_t num_geo;
  uint32_t geo_g, geo_d, geo_s; // green desert snow
  uint32_t geo_h, geo_m, geo_l; // entry points for medium and low geo lists
  uint32_t geoid[100];
  float *geo_off[100];          // every geometry (3do file) stores child offsets
  int geo_off_cnt[100];
  char draw[4];        // draw call id ("plot routine")
  char dram[4];        // medium..
  char dral[4];        // ..low detail variant
  char move[4];        // movement controller id ("move routine")
  int collidable;
  int projectile;
  int landable;
  int good_team_bad_target;
  int bad_team_good_target;
  int high_priority_target;
  int goal;
  float hitpoints;
  int type;                  // (0=UNK,1=TNK,2=AIR,3=COP,4=GUN,5=MSL)
  char description[16];      // short description for radar display
  // ambient sound would follow here
}
sx_object_t;

uint32_t c3_object_load(sx_object_t *o, const char *filename);
