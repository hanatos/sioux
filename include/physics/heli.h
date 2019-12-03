#pragma once
#include "rigidbody.h"
#include "aerofoil.h"
#include "world.h"

typedef enum sx_actuator_type_t
{
  s_act_gravity = 0,
  s_act_cyclic  = 1,
  s_act_tail    = 2,
  s_act_drag    = 3,
  s_act_damp    = 4,
  s_act_ext0    = 5,
  s_act_ext1    = 6,
  s_act_ext2    = 7,
  s_act_ext4    = 8,
  s_act_num     = 9
}
sx_actuator_type_t;

typedef struct sx_heli_control_t
{
  float cyclic[2];  // directional offset of main rotor
  float collective; // thrust of main rotor
  float tail;       // thrust of tail rotor

  int gear_move;    // 1 if expanding, -1 if retracting, 0 if no motion
  int flap_move;    // same as gear
  float gear;       // state of gear: 0 up 1 down
  float flap;       // state of weapons bay: 0 closed 1 open
}
sx_heli_control_t;

typedef enum sx_heli_damage_t
{
  s_dmg_none             = 0,
  s_dmg_main_rotor       = 1,
  s_dmg_main_rotor_fail  = 1<<1,
  s_dmg_tail_rotor       = 1<<2,
  s_dmg_tail_rotor_fail  = 1<<3,
  s_dmg_cannon           = 1<<4,
  s_dmg_cannon_fail      = 1<<5,
  s_dmg_engine           = 1<<6,
  s_dmg_engine_fail      = 1<<7,
  s_dmg_gear             = 1<<8,
  s_dmg_gear_fail        = 1<<9,
  s_dmg_flap             = 1<<10,
  s_dmg_flap_fail        = 1<<11,
  s_dmg_cargo_left       = 1<<12,
  s_dmg_cargo_left_fail  = 1<<13,
  s_dmg_cargo_right      = 1<<14,
  s_dmg_cargo_right_fail = 1<<14,
}
sx_heli_damage_t;

typedef enum sx_heli_surf_t
{
  s_heli_surf_vstab = 0,
  s_heli_surf_hstab_l,
  s_heli_surf_hstab_r,
  s_heli_surf_body_bl,
  s_heli_surf_body_br,
  s_heli_surf_body_tl,
  s_heli_surf_body_tr,
  s_heli_surf_body_fr,
  s_heli_surf_body_bk,
  s_heli_surf_cnt,
}
sx_heli_surf_t;

typedef struct sx_heli_t
{
  sx_heli_control_t ctl;
  sx_actuator_t act[s_act_num];
  sx_entity_t *entity;

  sx_aerofoil_t surf[s_heli_surf_cnt];

  float dim[3]; // dimensions of box used as flight model
  float cd[3];  // drag coefficients for main axes

  float mnr[3]; // main rotor center, with center of mass as zero
  float tlr[3]; // tail rotor center, with center of mass as zero
  float cmm[3]; // center of mass in model coordinates (it's not 0,0,0)

  sx_heli_damage_t damage; // individual damage flags

  // all these affect the inertia tensor. consider updating in-flight!
  float main_rotor_radius;
  float main_rotor_weight;
  float tail_rotor_weight;

  float cargo_left_weight;
  float cargo_right_weight;

  float useful_weight;  // hover with 70% torque at this weight
  float service_height; // max altitude
}
sx_heli_t;

void sx_heli_init(sx_heli_t *hl, sx_entity_t *ent);

float sx_heli_groundlevel(const sx_rigid_body_t *b);

float sx_heli_alt_above_ground(const sx_heli_t *h);

static inline void sx_heli_control_cyclic_x(sx_heli_t *h, float v)
{
  h->ctl.cyclic[0] = CLAMP(v, -1.0f, 1.0f);
}

static inline void sx_heli_control_cyclic_z(sx_heli_t *h, float v)
{
  h->ctl.cyclic[1] = CLAMP(v, -1.0f, 1.0f);
}

static inline void sx_heli_control_collective(sx_heli_t *h, float v)
{
  h->ctl.collective = 0.2 + CLAMP(v, -0.2f, 1.0f);
}

static inline void sx_heli_control_tail(sx_heli_t *h, float v)
{
  h->ctl.tail = CLAMP(v, -1.0f, 1.0f);
}

static inline void sx_heli_control_gear(sx_heli_t *h)
{
  if(h->ctl.gear_move >= 0 && h->ctl.gear == 1.0f) h->ctl.gear_move = -1;
  if(h->ctl.gear_move <= 0 && h->ctl.gear == 0.0f) h->ctl.gear_move =  1;
}

static inline void sx_heli_control_flap(sx_heli_t *h)
{
  if(h->ctl.flap_move >= 0 && h->ctl.flap == 1.0f) h->ctl.flap_move = -1;
  if(h->ctl.flap_move <= 0 && h->ctl.flap == 0.0f) h->ctl.flap_move =  1;
}

void sx_heli_update_forces(void *h, sx_rigid_body_t *b);

// damage the helicopter by given impulse p and hit point x (TODO: model or world space?)
void sx_heli_damage(void *h, float x[3], float p[3]);
