#include "sx.h"
#include "physics/rigidbody.h"
#include "world.h"
#include "physics/move.h"
#include "physics/obb_obb.h"
#include "move/common.h"
// helicopter move routines

typedef struct sx_move_helo_t
{
  // move physics/heli state here?

  // damage system, controlling flight dynamics:
  float dmg_main_rotor;
  float dmg_tail_rotor;
  float dmg_fuselage;
  float dmg_gear;
  float dmg_bay;
  float dmg_wing_tail;
  float dmg_wing_left;
  float dmg_wing_right;
}
sx_move_helo_t;

void
sx_move_helo_update_forces(sx_entity_t *e, sx_rigid_body_t *b)
{
  // TODO: wire into physics
}

void
sx_move_helo_damage(
    sx_entity_t       *e,
    const sx_entity_t *collider,
    float              extra_damage)
{
  // TODO: unify with comanche/heli.c!
  if(!collider)
  {
    // terrain:
    const float groundlevel = sx_world_get_height(e->body.c);
    const float top = /* center of mass offset?*/-4.0f-sx.assets.object[e->objectid].geo_aabb[0][1]; // just the body
    const float ht = groundlevel + top;

    if(e->body.c[1] >= ht) return;

    e->body.c[1] = ht;
    e->prev_x[1] = ht;

    float n[3]; // normal of terrain
    sx_world_get_normal(e->body.c, n);
    float up[3] = {0, 1, 0}, rg[3];
    quat_transform(&e->body.q, up); // to world space
    float dt = dot(up, n);
    cross(up, n, rg);
    // detect degenerate case where |rg| <<
    float len = dot(rg, rg);
    if(len > 0.02)
    {
      quat_t q0 = e->body.q;
      quat_t q1;
      quat_init_angle(&q1, acosf(fminf(1.0, fmaxf(0.0, dt))), rg[0], rg[1], rg[2]);
      quat_normalise(&q1);
      quat_mul(&q1, &q0, &e->body.q);
    }
    // remove momentum
    for(int k=0;k<3;k++)
      e->body.pw[k] = 0.0f;
    e->body.pv[1] = 0.0f;
    e->body.pv[0] *= 0.5f;
    e->body.pv[2] *= 0.5f;
    return;
  }
  // call into the move routines to obtain list of collidable obb with
  // sx_part_type_t description what it is
  sx_obb_t obb[3];
  sx_part_type_t pt[3];
  int np = e->plot.collide ? e->plot.collide(e, obb, pt) : 0;

  // TODO: create appropriate damage to our system
  // TODO: apply forces
  sx_obb_t box1;
  sx_obb_get(&box1, collider, 0, -1); // get first element, without offset since we don't know any better
  for(int p=0;p<np;p++)
  {
    if(sx_collision_test_obb_obb(&box1, obb+p))
    {
      // fprintf(stderr, "collision %s -- %s hit part type %d\n",
      //     sx.assets.object[e->objectid].filename,
      //     sx.assets.object[collider->objectid].filename,
      //     pt[p]);
      e->hitpoints -= 40;
      sx_sound_play(sx.assets.sound + sx.mission.snd_hit, -1);
    }
  }
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
}

