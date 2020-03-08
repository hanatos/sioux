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
  float last_p[3];
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
    float c = 80000.0f;
    sx_actuator_t thrust = {
      .f = {0.0f, 0.0f, c},
      .r = {0.0f, 0.0f, 0.0f},
    };
    quat_transform(&b->q, thrust.f);
    sx_rigid_body_apply_force(b, &thrust);
    if(e->parent && e->parent->engaged != -1u)
    {
      float d[] = {
        sx.world.entity[e->parent->engaged].body.c[0] - e->body.c[0],
        sx.world.entity[e->parent->engaged].body.c[1] - e->body.c[1],
        sx.world.entity[e->parent->engaged].body.c[2] - e->body.c[2]};
      c *= .3;
      normalise(d);
      d[1] += 0.2;
      normalise(d);
      sx_actuator_t steer = {
        .f = {c*d[0], c*d[1], c*d[2]},
        .r = {0.0f, 0.0f, 0.0f},
      };
      sx_rigid_body_apply_force(b, &steer);
    }
  }
}

void
sx_move_rock_damage(sx_entity_t *e, const sx_entity_t *c, float dmg)
{
  // deal only with terrain, the others will collide against us
  if(!c)
  {
    const float groundlevel = sx_world_get_height(e->body.c);
    const float top = -sx.assets.object[e->objectid].aabb[1];
    const float ht = groundlevel + top;
    if(e->body.c[1] >= ht) return;
    // sx_move_rock_t *r = e->move_data;
    // TODO: deal damage?
    // TODO: if height < terrain, spawn dirt explosion instead
    sx_spawn_explosion(e); // spawn explosion
    // and a few chunks of debris
    // XXX do that on the other object, or else they continue to fly with the rocket
    // for(int i=0;i<10;i++) sx_spawn_debris(e);
    sx_world_remove_entity(e - sx.world.entity); // remove us
  }
}

// 
void
sx_move_rock_think(sx_entity_t *e)
{
  sx_move_rock_t *r = e->move_data;
  // run out of fuel after a while and then fall down and explode later
  if(r->fuel > 0) r->fuel--;
  if((r->fuel & 7) == 7)
  {
    if(r->last_p[0] != -1.0)
      sx_spawn_trail(e, r->last_p);
    for(int k=0;k<3;k++)
      r->last_p[k] = e->body.c[k];
  }
}

void
sx_move_rock_init(sx_entity_t *e)
{
  sx_move_rock_t *r = malloc(sizeof(*r));
  r->fuel = 200;
  for(int k=0;k<3;k++) r->last_p[k] = -1.0;
  e->move_data = r;
}

