#include "sx.h"
#include "world.h"
#include "move/common.h"
#include "physics/obb_obb.h"
#include "gameplay.h"

void
sx_move_default_damage(sx_entity_t *e, const sx_entity_t *coll, float dmg)
{
  float new_hitpoints = e->hitpoints;
  if(coll)
  {
    if(e->hitpoints <= 0) return; // no need to precisely intersect dead things
    // call into the move routines to obtain list of collidable obb with
    // sx_part_type_t description what it is
    sx_obb_t col_obb[3];
    sx_part_type_t col_pt[3] = {0};
    sx_obb_t our_obb[3];
    sx_part_type_t our_pt[3] = {0};
    int our_np = 1, col_np = 1;
    if(e->plot.collide)
      our_np = e->plot.collide(e, our_obb, our_pt);
    else
      sx_obb_get(our_obb, e, 0, -1); // first element without offset

    if(coll->plot.collide)
      col_np = coll->plot.collide(coll, col_obb, col_pt);
    else
      sx_obb_get(col_obb, coll, 0, -1); // first element without offset

    for(int op=0;op<our_np;op++) for(int cp=0;cp<col_np;cp++)
    {
      if(sx_collision_test_obb_obb(col_obb+cp, our_obb+op))
      {
        new_hitpoints -= 4;
        // sx_sound_play(sx.assets.sound + sx.mission.snd_hit, -1);
        if(e->hitpoints > 0 && new_hitpoints <= 0)
        {
          // sx_spawn_explosion(e);
          // for(int i=0;i<10;i++) sx_spawn_debris(e);
          // and also set objectid to something broken..
          if(e->objectid < sx.assets.num_objects)
            e->objectid ++;
        }
        // fprintf(stderr, "[coll] ent %zu p %d -- %zu %d\n", e-sx.world.entity, op, coll - sx.world.entity, cp);
        fprintf(stderr, "[coll] %s's part %d hit by %s's %d hitp %g\n",
             sx.assets.object[e->objectid].filename,
             our_pt[op],
             sx.assets.object[coll->objectid].filename,
             col_pt[cp],
             new_hitpoints);
        e->hitpoints = new_hitpoints;
        return;
      }
    }
    return;
  }
  // else terrain:
  const float groundlevel = sx_world_get_height(e->body.c);
  const float top = -sx.assets.object[e->objectid].aabb[1]; // full shape, object space
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
}
