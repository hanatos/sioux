#pragma once
#include "move/common.h"

// some global gameplay calls:

// launch child objects: 
// fire, explosion, smoke, debris, rockets, missiles, gunfire, ..
static inline uint32_t
sx_spawn_child(uint32_t obj, sx_entity_t *p)
{
  uint32_t eid = sx_world_add_entity(
    p, obj, p->body.c, &p->body.q,
    0, 0);
  if(eid == -1) return -1; // no more free slots
  return eid;
}

static inline uint32_t
sx_spawn_explosion(
    sx_entity_t *parent)
{
  sx_sound_play(sx.assets.sound + sx.mission.snd_explode, -1);
  uint32_t eid = sx_spawn_child(sx.mission.obj_explosion_nuke, parent);
  if(eid == -1) return -1;
  // reset impulse and give it a slight upwards motion
  for(int k=0;k<3;k++)
    sx.world.entity[eid].body.pv[k] = 0.0f;
  for(int k=0;k<3;k++)
    sx.world.entity[eid].body.pw[k] = 0.1f;
  sx.world.entity[eid].body.pv[1] = 0.3f;
  return eid;
}

static inline uint32_t
sx_spawn_rocket(
    sx_entity_t *parent)
{
  // play rocket sound, fade with distance to camera
  // TODO: sx_sound_play();
  return sx_spawn_child(sx.mission.obj_rocket, parent);
}

static inline uint32_t
sx_spawn_trail(
    sx_entity_t *parent,
    const float *prev)
{
  return sx_spawn_child(sx.mission.obj_trail, parent);
  // TODO: now setup rotation matrix to spawn aabb[2] from prev -> parent->body.c
}

static inline uint32_t
sx_spawn_debris(
    sx_entity_t *parent)
{
  return sx_spawn_child(sx.mission.obj_debris, parent);
}

static inline uint32_t
sx_spawn_fire(
    sx_entity_t *parent)
{
  return sx_spawn_child(sx.mission.obj_fire, parent);
}


// trigger kill (lose)
// trigger pad/flight/win
