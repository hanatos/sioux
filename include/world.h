#pragma once
#include "physics/rigidbody.h"
#include "physics/move.h"
#include "physics/grid.h"

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

// movement controller for user or ai input
typedef struct sx_move_controller_t
{
  uint32_t time;         // timestamp of this input, for networking
  float throttle;
  float collective;      // we'll ignore throttle for now
  float cyclic[2];
  float tail;
  int trigger_gear;      // 1 expand, -1 retract, 0 no motion
  int trigger_bay;       // 1 open, -1 close, 0 no motion
  int trigger_fire;      // 1 if firing

  int autopilot_alt;             // keep altitude above ground at target level
  int autopilot_vel;             // keep world space velocity at target
  int autopilot_rot;             // keep yaw rotation at zero (tail rotor)
  float autopilot_alt_target;    // target altitude above ground level
  float autopilot_vel_target[3]; // target velocity in world space
  float autopilot_alt_guess;     // current guess of zero-vert-vel collective
  float autopilot_vel_guess[2];  // current guess of nose orientation
}
sx_move_controller_t;

// this is useful for hud display or ai feedback
typedef struct sx_move_feedback_t
{
  float alt_above_ground; // altitude above ground level
  float speed;            // speed over ground
  float vel[3];           // velocity vector over ground
  float vspeed;           // vertical speed
  float gear;             // state of gear: 0 up, 1 down
  float bay;              // state of bay door: 0 closed, 1 open
  // add information about stall, rotor revolutions, fuel load, etc
}
sx_move_feedback_t;

typedef struct sx_entity_t
{
  sx_rigid_body_t body;     // rigid body for physics sim
  uint32_t objectid;        // references into assets for rendering
  uint32_t engaged;         // targeted this entity
  sx_entity_t *parent;      // parent entity if any, or 0 // XXX id for network?

  sx_move_t move;           // movement controller with callbacks
  void     *move_data;      // dynamically allocated movement controller data
  sx_move_controller_t ctl; // user or ai input for movement
  sx_move_feedback_t stat;  // status of movement

  sx_plot_t plot;           // wrap plot functions

  quat_t prev_q;            // previous alignment
  float  prev_x[3];         // previous position
  float  cmm[3];            // center of mass in model coordinates, used to offset rendering

  char    id;               // single letter identifying group for trigger scripting
  uint8_t camp;             // which side are you on?

  uint32_t curr_wp;         // current waypoint
  uint32_t prev_wp;         // previous waypoint
  uint32_t weapon;          // currently equipped weapon

  uint32_t birth_time;      // when was this spawned?
  float hitpoints;          // how much damage can we still take?

  uint32_t host_owner;      // the host id that owns this entity in networking
}
sx_entity_t;

typedef struct sx_group_t
{
  uint8_t      camp;       // same as entities
  uint32_t     num_members;
  sx_entity_t *member[300];

  uint32_t     num_waypoints;
  float        waypoint[32][2];
}
sx_group_t;

typedef struct sx_world_t
{
  // TODO: store current mission here?

  // list of loaded entities, inited from .pos file when entering mission
  uint32_t num_entities;
  uint32_t num_static_entities;
  sx_entity_t entity[1024];

  // entities are grouped into squadrons etc:
  uint32_t num_groups;
  sx_group_t group[256];

  int32_t terrain_wd, terrain_ht, terrain_bpp;
  uint8_t *terrain; // TODO: make 16 bit?
  float terrain_low, terrain_high, cloud_height;

  // TODO: list of movement controllers for dynamic entities?
  // these may need state (fuel left in rocket, advanced inertia tensors etc)

  // TODO: wrap this up in one struct:
  // that is the one with the "plyr" movement routine attached
  uint32_t player_entity;

  // for collision detection
  sx_grid_t grid;
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
