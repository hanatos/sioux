#pragma once

// auto-generate all headers:
#define MOVE_LIST \
MOVE_DO(rock)\
MOVE_DO(toss)\
MOVE_DO(helo)\
MOVE_DO(boom)

#define MOVE_DO(A)\
void sx_move_ ## A ## _init(sx_entity_t *e);\
void sx_move_ ## A ## _damage(sx_entity_t *e, float x[3], float p[3]);\
void sx_move_ ## A ## _update_forces(sx_entity_t *e, sx_rigid_body_t *b);\
void sx_move_ ## A ## _think(sx_entity_t *e);

MOVE_LIST
#undef MOVE_LIST

static inline void
sx_move_init(sx_entity_t *e)
{
  const char *move = sx.assets.object[e->objectid].move;
  if(!strncmp(move, "helo", 4))
    return sx_move_helo_init(e);
  if(!strncmp(move, "rock", 4))
    return sx_move_rock_init(e);
  if(!strncmp(move, "boom", 4))
    return sx_move_boom_init(e);
  if(!strncmp(move, "toss", 4))
    return sx_move_toss_init(e);
}
