#pragma once
#include "move/common.h"

// some global gameplay calls:

// TODO:
// launch child objects: 
// fire, explosion, smoke, debris, rockets, missiles, gunfire
static inline uint32_t
sx_spawn_child(uint32_t obj, sx_entity_t *p)
{
  uint32_t eid = sx_world_add_entity(
    obj, -1,
    p->body.c, &p->body.q,
    0, 0);
  if(eid == -1) return -1; // no more free slots
  // in fact don't ground:
  for(int k=0;k<3;k++) sx.world.entity[eid].prev_x[k] = p->body.c[k];
  sx.world.entity[eid].prev_q = p->body.q;
  for(int k=0;k<3;k++) sx.world.entity[eid].body.c[k] = p->body.c[k];
  sx.world.entity[eid].body.q = p->body.q;
  float mr = sx.world.entity[eid].body.m / p->body.m;
  // init dependent entity's moments
  for(int k=0;k<3;k++)
    sx.world.entity[eid].body.pv[k] = p->body.pv[k] * mr;
  for(int k=0;k<3;k++)
    sx.world.entity[eid].body.pw[k] = p->body.pw[k] * mr;
  sx_move_init(sx.world.entity + eid);
  return eid;
}

static inline void
sx_spawn_explosion(
    sx_entity_t *parent)
{
  sx_sound_play(sx.assets.sound + sx.mission.snd_explode, -1);
  uint32_t eid = sx_spawn_child(sx.mission.obj_explosion_dirt, parent);
  if(eid == -1) return;
  // reset impulse and give it a slight upwards motion
  for(int k=0;k<3;k++)
    sx.world.entity[eid].body.pv[k] = 0.0f;
  for(int k=0;k<3;k++)
    sx.world.entity[eid].body.pw[k] = 0.1f;
  sx.world.entity[eid].body.pv[1] = 0.3f;
  const float groundlevel = sx_world_get_height(sx.world.entity[eid].body.c);
  sx.world.entity[eid].body.c[1] =
    MAX(sx.world.entity[eid].body.c[1], groundlevel + 2.5);
}

static inline void
sx_spawn_rocket(
    sx_entity_t *parent)
{
  // play rocket sound, fade with distance to camera
  // TODO: sx_sound_play();
  sx_spawn_child(sx.mission.obj_rocket, parent);
}

static inline void
sx_spawn_debris(
    sx_entity_t *parent)
{
  sx_spawn_child(sx.mission.obj_debris, parent);
}

static inline void
sx_spawn_fire(
    sx_entity_t *parent)
{
  sx_spawn_child(sx.mission.obj_fire, parent);
}


// trigger kill (lose)
// trigger pad/flight/win
