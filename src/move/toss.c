// toss move routines (for debris parts)
#include "sx.h"
#include "util.h"
#include "physics/rigidbody.h"
#include "world.h"
#include "physics/move.h"
#include "move/common.h"

typedef struct sx_move_toss_t
{
  int timeout;
}
sx_move_toss_t;

void
sx_move_toss_update_forces(sx_entity_t *e, sx_rigid_body_t *b)
{
  sx_move_toss_t *r = e->move_data;
  if(r->timeout >= 198)
  {
    uint32_t seed[] = {e - sx.world.entity, 0};
    float rand[2], str = 50.0;
    encrypt_tea(seed, rand);
    sx_actuator_t push = {
      .f = {str*(1.0-2.0f*rand[0]), str, str*(1.0f-2.0f*rand[1])},
      .r = {0.0f, 0.0f, 0.0f},
    };
    sx_rigid_body_apply_force(b, &push);
  }
  const int m = 10.0f;
  sx_actuator_t grav = { // gravity
    .f = {0.0f, -9.81f * m, 0.0f},
    .r = {0.0f, 0.0f, 0.0f},
  };
  sx_rigid_body_apply_force(b, &grav);
}

void
sx_move_toss_damage(sx_entity_t *e, const sx_entity_t *c, float dmg)
{
  // TODO: wire
}

void
sx_move_toss_think(sx_entity_t *e)
{
  sx_move_toss_t *r = e->move_data;
  r->timeout--;
  if(r->timeout <= 0)
    sx_world_remove_entity(e - sx.world.entity); // remove us
}

void
sx_move_toss_init(sx_entity_t *e)
{
  sx_move_toss_t *r = malloc(sizeof(*r));
  memset(r, 0, sizeof(*r));
  e->move_data = r;
  r->timeout = 200;
}

