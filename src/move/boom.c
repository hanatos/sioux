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
  // TODO: wire into physics
}

void
sx_move_boom_damage(sx_entity_t *e, float x[], float p[])
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
  r->timeout = 32;
  e->move = (sx_move_t){
    .id            = "boom",
    .update_forces = sx_move_boom_update_forces,
    .damage        = sx_move_boom_damage,
    .think         = sx_move_boom_think,
    .snd_ambient   = sx_assets_load_sound(&sx.assets, "explode.wav"),
  };
}

