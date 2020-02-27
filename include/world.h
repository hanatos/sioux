#pragma once
#include "physics/rigidbody.h"
#include "physics/move.h"
#include "physics/accel.h"

#include <stdint.h>

// wraps current state of the world

// global list of parts of entities, such that
// we can deal damage and instruct the move routine
// to adapt the physics sim accordingly.
typedef enum sx_part_type_t
{
  s_part_body,
  s_part_main_rotor,
  s_part_tail_rotor,
  s_part_tail_stab,
  s_part_weap_right,
  s_part_weap_left,
  s_part_gear,
  s_part_bay,
}
sx_part_type_t;

typedef struct sx_obb_t sx_obb_t;
typedef struct sx_plot_t
{
  char id[4];
  void (*plot)(uint32_t ei);
  int  (*collide)(const sx_entity_t *e, sx_obb_t *box, sx_part_type_t *pt);
}
sx_plot_t;

typedef struct sx_entity_t
{
  // rigid body can be applied forces (need to transform to local space)
  sx_rigid_body_t body;
  uint32_t objectid;      // references into assets
  sx_move_t move;         // movement controller with callbacks
  void *move_data;        // dynamically allocated movement controller data

  sx_plot_t plot;         // wrap plot functions

  // previous position and alignment
  quat_t prev_q;
  float prev_x[3];

  char id;      // single letter identifying group for trigger scripting
  uint8_t camp; // which side are you on?

  uint32_t birth_time; // when was this spawned?
  float hitpoints;     // how much damage can we still take?
}
sx_entity_t;

typedef struct sx_heli_t sx_heli_t;

typedef struct sx_world_t
{
  // TODO: store current mission here?

  // list of loaded entities, inited from .pos file when entering mission
  uint32_t num_entities;
  sx_entity_t entity[1024];

  int32_t terrain_wd, terrain_ht, terrain_bpp;
  uint8_t *terrain; // TODO: make 16 bit?
  float terrain_low, terrain_high, cloud_height;

  // TODO: list of movement controllers for dynamic entities?
  // these may need state (fuel left in rocket, advanced inertia tensors etc)

  // TODO: wrap this up in one struct:
  // that is the one with the "plyr" movement routine attached
  uint32_t player_entity;
  sx_heli_t *player_move;  // TODO: have one for every entity with a given move routine
  uint32_t player_wp;      // index of next waypoint (in mission or here?)
  uint32_t player_old_wp;
  uint32_t player_weapon;

  uint32_t fire_entity;    // broken stuff burns

  // for collision detection
  sx_bvh_t *bvh;
}
sx_world_t;

void sx_world_init();
void sx_world_cleanup();

void sx_world_think(const uint32_t dt);

// collision detection and interaction between objects
void sx_world_move(const uint32_t dt);

// setup terrain height map for collision detection
int sx_world_init_terrain(const char *filename,
    uint32_t elevation, uint32_t cloud_height, uint32_t terrain_scale);

void sx_world_render_entity(uint32_t ei);

void sx_world_remove_entity(uint32_t ei);

// TODO: probably needs /a lot/ of amending
uint32_t sx_world_add_entity(
    uint32_t objectid,
    const float *pos, const quat_t *q,
    char id, uint8_t camp);

float sx_world_get_height(const float *p);
void  sx_world_get_normal(const float *p, float *n);
