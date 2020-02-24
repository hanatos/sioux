#include "sx.h"
#include "physics/rigidbody.h"
#include "world.h"
#include "physics/move.h"
#include "move/common.h"
#include "gameplay.h"
// rocket move routine

typedef struct sx_move_rock_t
{
  int fuel;
}
sx_move_rock_t;

void
sx_move_rock_update_forces(sx_entity_t *e, sx_rigid_body_t *b)
{
  sx_move_rock_t *r = e->move_data;
  const int m = 100.0f + 10.0f*r->fuel;
  sx_actuator_t grav = { // gravity
    .f = {0.0f, -9.81f * m, 0.0f},
    .r = {0.0f, 0.0f, 0.0f},
  };
  sx_rigid_body_apply_force(b, &grav);
  if(r->fuel > 0)
  { // thrust of rocket booster
    sx_actuator_t thrust = {
      .f = {0.0f, 0.0f, 80000.0f},
      .r = {0.0f, 0.0f, 0.0f},
    };
    quat_transform(&b->q, thrust.f);
    sx_rigid_body_apply_force(b, &thrust);
  }
}

void
sx_move_rock_damage(sx_entity_t *e, float x[], float p[])
{
  // sx_move_rock_t *r = e->move_data;
  // TODO: deal damage?
  // TODO: if height < terrain, spawn dirt explosion instead
  sx_spawn_explosion(e); // spawn explosion
  // and a few chunks of debris
  // XXX do that on the other object, or else they continue to fly with the rocket
  // for(int i=0;i<10;i++) sx_spawn_debris(e);
  sx_world_remove_entity(e - sx.world.entity); // remove us
}

// 
void
sx_move_rock_think(sx_entity_t *e)
{
  sx_move_rock_t *r = e->move_data;
  // run out of fuel after a while and then fall down and explode later
  if(r->fuel > 0) r->fuel--;
}

void
sx_move_rock_init(sx_entity_t *e)
{
  sx_move_rock_t *r = malloc(sizeof(*r));
  r->fuel = 100;
  e->move_data = r;
  e->move = (sx_move_t){
    .id            = "rock",
    .update_forces = sx_move_rock_update_forces,
    .damage        = sx_move_rock_damage,
    .think         = sx_move_rock_think,
    .snd_ambient   = sx_assets_load_sound(&sx.assets, "missile.wav"),
  };
}

