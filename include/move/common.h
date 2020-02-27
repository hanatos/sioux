#pragma once

// auto-generate all headers:
#define MOVE_LIST \
MOVE_DO(rock)\
MOVE_DO(toss)\
MOVE_DO(helo)\
MOVE_DO(boom)

#define MOVE_DO(A)\
void sx_move_ ## A ## _init(sx_entity_t *e);\
void sx_move_ ## A ## _damage(sx_entity_t *e, const sx_entity_t *coll, float dmg);\
void sx_move_ ## A ## _update_forces(sx_entity_t *e, sx_rigid_body_t *b);\
void sx_move_ ## A ## _think(sx_entity_t *e);

MOVE_LIST
#undef MOVE_DO

static inline void
sx_move_default_damage(sx_entity_t *e, const sx_entity_t *coll, float dmg)
{
  if(coll) return; // don't handle object collisions
  // terrain:
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

static inline void
sx_move_init(sx_entity_t *e)
{
  const char *move = sx.assets.object[e->objectid].move;
#define MOVE_DO(A)\
  if(!strncmp(move, #A, 4)) {\
    e->move = (sx_move_t){\
      .id            = #A,\
      .update_forces = sx_move_ ## A ## _update_forces,\
      .damage        = sx_move_ ## A ## _damage,\
      .think         = sx_move_ ## A ## _think,\
    };\
    return sx_move_ ## A ## _init(e);\
  }
  MOVE_LIST
#undef MOVE_DO
  e->move = (sx_move_t){
    .id     = "none",
    .damage = sx_move_default_damage,
  };
}
#undef MOVE_LIST
