#include "sx.h"
#include "physics/rigidbody.h"
#include "world.h"
#include "physics/move.h"
#include "move/common.h"
// helicopter move routines

typedef struct sx_move_helo_t
{
  // move physics/heli state here?
}
sx_move_helo_t;

void
sx_move_helo_update_forces(sx_entity_t *e, sx_rigid_body_t *b)
{
  // TODO: wire into physics
}

void
sx_move_helo_damage(sx_entity_t *e, float x[], float p[])
{
  // TODO: wire
}

void
sx_move_helo_think(sx_entity_t *e)
{
  // TODO: wire
}

void
sx_move_helo_init(sx_entity_t *e)
{
  sx_move_helo_t *r = malloc(sizeof(*r));
  memset(r, 0, sizeof(*r));
  e->move_data = r;
  e->move = (sx_move_t){
    .id            = "helo",
    .update_forces = sx_move_helo_update_forces,
    .damage        = sx_move_helo_damage,
    .think         = sx_move_helo_think,
    // .snd_ambient   = sx_assets_load_sound(&sx.assets, "comanche.wav"),
  };
}

