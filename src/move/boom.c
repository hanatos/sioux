#include "sx.h"
#include "physics/rigidbody.h"
#include "world.h"
#include "physics/move.h"
#include "move/common.h"
// explosion move routines

typedef struct sx_move_boom_t
{
  int timeout;
}
sx_move_boom_t;

void
sx_move_boom_update_forces(sx_entity_t *e, sx_rigid_body_t *b)
{
  // these are all dust plumes, splashes, explosions, rocket trails, ..
  // and slow down very quickly, so all we have is drag:
  // force = 0.5 * air density * |u|^2 * A(u) * cd(u)
  // where u is the relative air/object speed, cd the drag coefficient and A the area
  // mass density kg/m^3 of air at 20C
  float drag = 0;
  const float rho_air = 1.204;
  float wind[3] = {0};
  for(int k=0;k<3;k++) wind[k] -= b->v[k];
  float vel2 = dot(wind, wind);
  if(vel2 > 0.0f)
  {
    const float cd = 2.5f;    // isotropic drag coefficient
    drag = cd * .5 * rho_air * vel2;
    sx_actuator_t a = {
      .r = {0},
      .f = {wind[0]*drag, wind[1]*drag, wind[2]*drag}};
    sx_rigid_body_apply_force(b, &a);
  }
}

void
sx_move_boom_damage(sx_entity_t *e, const sx_entity_t *c, float dmg)
{
  // TODO: wire
}

void
sx_move_boom_think(sx_entity_t *e)
{
  sx_move_boom_t *r = e->move_data;
  r->timeout--;
  if(r->timeout <= 0)
    sx_world_remove_entity(e - sx.world.entity); // remove us
}

void
sx_move_boom_init(sx_entity_t *e)
{
  sx_move_boom_t *r = malloc(sizeof(*r));
  memset(r, 0, sizeof(*r));
  e->move_data = r;
  r->timeout = 200;
}

