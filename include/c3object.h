#pragma once
#include <stdint.h>

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
  char draw[4];        // draw call id
  char dram[4];        // medium..
  char dral[4];        // ..low detail variant
  char move[4];        // movement controller id
#if 0 // TODO: extra flags, hitpoints, etc
<
1       ;Collidable
0       ;Missile / Projectile
0       ;Landable - You can land on the top of this object

0       ;Good Team - Bad Target
0       ;Bad Team - Good Target
1       ;High Priority Target - Friendly Fire Systems
1       ;Goal

25      ;hit points
3       ;(0=UNK,1=TNK,2=AIR,3=COP,4=GUN,5=MSL)
APACHE
<
#endif
}
sx_object_t;

uint32_t c3_object_load(sx_object_t *o, const char *filename);
