#pragma once

// this wraps the "move routines" found in the .ai files of c3.
// we'll also assume that ambient sounds and damage controls are
// included in this concept.

// generic interface functions:

// TODO: interface to return list of points to check for collision with boxes
// (which can then compute their damage themselves, so we don't need the damage
// callback?
// still need to pipe back the feedback which point was collided (to do damage)
// excert some force can be done via generic rigid body interface
typedef void (*sx_move_damage_t)(void *move, float x[3], float p[3]);

// user input controller? SDL_Event? abstraction?

// update forces (will be used by runge-kutta integration)
typedef void (*sx_move_update_forces_t)(void *move, sx_rigid_body_t *b);

// setup inertia tensor?

typedef struct sx_move_t
{
  char id[4]; // this movement controller's 4-character id
  sx_move_damage_t damage;
  sx_move_update_forces_t update_forces;
  uint32_t snd_ambient;
}
sx_move_t;
