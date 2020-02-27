#pragma once

// this wraps the "move routines" found in the .ai files of c3.
// we'll also assume that ambient sounds and damage controls are
// included in this concept.


typedef struct sx_entity_t sx_entity_t;

// generic interface functions:

// TODO: interface to return list of points to check for collision with boxes
// (which can then compute their damage themselves, so we don't need the damage
// callback?
// still need to pipe back the feedback which point was collided (to do damage)
// excert some force can be done via generic rigid body interface
typedef void (*sx_move_damage_t)(sx_entity_t *e, const sx_entity_t *c, float dmg);

// user input controller? SDL_Event? abstraction?

// update forces (will be used by runge-kutta integration)
typedef void (*sx_move_update_forces_t)(sx_entity_t *e, sx_rigid_body_t *b);

// init movement controller, set e->move_data to payload
typedef void (*sx_move_init_t)(sx_entity_t *e);

// place to implement object AI
typedef void (*sx_move_think_t)(sx_entity_t *e);

// setup inertia tensor?

typedef struct sx_move_t
{
  char id[4]; // this movement controller's 4-character id
  sx_move_init_t          init;
  sx_move_damage_t        damage;
  sx_move_update_forces_t update_forces;
  sx_move_think_t         think;
  uint32_t snd_ambient;
}
sx_move_t;
