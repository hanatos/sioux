#pragma once

// auto-generate all headers:
#define MOVE_LIST \
MOVE_DO(rock)\
MOVE_DO(hell)\
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

void
sx_move_default_damage(sx_entity_t *e, const sx_entity_t *coll, float dmg);

static inline void
sx_move_init(sx_entity_t *e)
{
  const char *move = sx.assets.object[e->objectid].move;

  // init default inertia tensor
  const float rho = 50.0f; // mass density
  // derived quantities:
  // TODO: this the good bounding box?
  // const float *aabb = sx.assets.object[objectid].geo_aabb[0]; // just the body
  const float w = 2.0f, l = 2.0f, h = 3.0f; // [m]
  // init inertia tensor
  //                | y^2 + z^2  -xy         -xz        |
  // I = int rho dV |-xy          x^2 + z^2  -xz        |
  //                |-xz         -yz          y^2 + x^2 |
  const float mass = rho * w * h * l;
  e->body.m = mass;
  e->body.invI[0] = 3.0f/8.0f / (rho * w * (h * l*l*l + l * h*h*h));
  e->body.invI[4] = 3.0f/8.0f / (rho * h * (w * l*l*l + l * w*w*w));
  e->body.invI[8] = 3.0f/8.0f / (rho * l * (w * h*h*h + h * w*w*w));

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
  if(!strncmp(move, "wing", 4)) {
    e->move = (sx_move_t){
      .id            = "wing",
      .update_forces = sx_move_helo_update_forces,
      .damage        = sx_move_helo_damage,
      .think         = sx_move_helo_think, // TODO: put different thing here
    };
    return sx_move_helo_init(e);
  }
  if(!strncmp(move, "plyr", 4)) {
    e->move = (sx_move_t){
      .id            = "plyr",
      .update_forces = sx_move_helo_update_forces,
      .damage        = sx_move_helo_damage,
      .think         = sx_move_helo_think, // TODO: put different thing here
    };
    return sx_move_helo_init(e);
  }
  e->move = (sx_move_t){
    .id     = "none",
    .damage = sx_move_default_damage,
  };
}
#undef MOVE_LIST
