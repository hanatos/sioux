#pragma once
#include "physics/rigidbody.h"
#include "physics/move.h"

#include <stdint.h>

// wraps current state of the world

typedef struct sx_entity_t
{
  // rigid body can be applied forces (need to transform to local space)
  sx_rigid_body_t body;
  uint32_t objectid;      // references into assets
  uint32_t dead_objectid; // this is the object if the entity was destroyed
  sx_move_t move;         // movement controller with callbacks
  void *move_data;        // dynamically allocated movement controller data

  // previous position and alignment
  quat_t prev_q;
  float prev_x[3];

  char id;      // single letter identifying group for trigger scripting
  uint8_t camp; // which side are you on?

  float hitpoints; // how much damage can we still take?
}
sx_entity_t;

typedef struct sx_heli_t sx_heli_t;

typedef struct sx_world_t
{
  // global things:
  // wind conditions
  // terrain texture ids
  // sun sky clouds environment

  // TODO: store current mission here?

  // list of loaded entities, inited from .pos file when entering mission
  uint32_t num_entities;
  sx_entity_t entity[1024];

  // pool for dynamically spawned entities:
  uint32_t num_dyn_entities;

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
}
sx_world_t;

void sx_world_init();
void sx_world_cleanup();

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
    uint32_t dead_objectid,
    float *pos, quat_t *q,
    char id, uint8_t camp, uint32_t ground);

float sx_world_get_height(const float *p);
void  sx_world_get_normal(const float *p, float *n);
