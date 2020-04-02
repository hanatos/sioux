#pragma once

#include "quat.h"
#include "matrix3.h"
#include <assert.h>

// fwd declare
void sx_vid_add_debug_line(const float *v0, const float *v1);

typedef struct sx_actuator_t
{
  float r[3];      // coordinates where force is applied, object space [m], in particular relative to center of mass
  float f[3];      // 3d force with direction and magnitude in [N = kg m / s^2]
}
sx_actuator_t;

typedef struct sx_rigid_body_t
{
  // primary quantities:
  float c[3];      // center of mass in world space [m]
  quat_t q;        // orientation object to world space
  float pv[3];     // momentum in world space
  float pw[3];     // angular momentum in world space

  // constant quantities:
  float invI[9];   // inverse inertia tensor in object space
  float m;         // total mass

  // derived quantities:
  float v[3];      // velocity in world space [m/s]
  float w[3];      // angular velocity in world space [rad/s]
  quat_t spin;     // world space spin (dq/dt)

  // collected forces:
  float torque[3]; // collected rotational force (= r x f)
  float force[3];  // collected linear force (both in world space)

  // hack to keep things slow
  float angular_drag;  // pw *= (1-angular_drag)
  float drag;          // pv *= (1-drag)
}
sx_rigid_body_t;

// accumulate torque only
static inline void sx_rigid_body_apply_torque(
    sx_rigid_body_t *b,
    const float *torque)  // torque = r x f
{
  for(int k=0;k<3;k++) b->torque[k] += torque[k];
}

// accumulate linear force and torque
static inline void sx_rigid_body_apply_force(
    sx_rigid_body_t *b,
    const sx_actuator_t *act)
{
  // collect actuator force, linear part:
  for(int k=0;k<3;k++) b->force[k] += act->f[k];

#if 1
  float v0[3], v1[3];
  for(int k=0;k<3;k++) v0[k] = b->c[k] + act->r[k];
  for(int k=0;k<3;k++) v1[k] = b->c[k] + act->r[k] + act->f[k];
  sx_vid_add_debug_line(v0, v1);
#endif

  float fw[3]; // torque or angular force
  cross(act->r, act->f, fw);
  sx_rigid_body_apply_torque(b, fw);

  for(int k=0;k<3;k++) assert(act->f[k] == act->f[k]);
  for(int k=0;k<3;k++) assert(act->r[k] == act->r[k]);
}

static inline void sx_rigid_body_recalculate(
    sx_rigid_body_t *b)
{
  // update derived quantities in new state:
  quat_normalise(&b->q);

  for(int k=0;k<3;k++) b->v[k] = b->pv[k] / b->m; // integrate velocity in world space

  // transform inverse inertia tensor invI to world space,
  // compute angular velocity b->w in world space
  float R[9], tmp[3], tmp2[3];
  quat_to_mat3(&b->q, R);
  mat3_tmulv(R, b->pw, tmp);
  mat3_mulv (b->invI, tmp, tmp2);
  mat3_mulv (R, tmp2, b->w);

  // spin = dt * 0.5 * w * q;
  quat_t wq;
  quat_init(&wq, 0.0f, b->w[0], b->w[1], b->w[2]);
  quat_mul(&wq, &b->q, &b->spin);
  quat_muls(&b->spin, 0.5f);
}

static inline void sx_rigid_body_eval(
    sx_rigid_body_t *b,            // rigid body to update
    const sx_rigid_body_t *deriv,  // the one with the derivatives to use
    float dt)                      // time step
{
  // integrate angular velocity -> orientation
  quat_t spin_dt = deriv->spin;
  quat_muls(&spin_dt, dt);

  quat_add(&b->q, &spin_dt);
  quat_normalise(&b->q);

  for(int k=0;k<3;k++) b->c[k] += dt * deriv->v[k]; // integrate position in world space

  for(int k=0;k<3;k++) b->pw[k] += deriv->torque[k] * dt; // integrate angular momentum in world space
  for(int k=0;k<3;k++) b->pv[k] += deriv->force[k]  * dt; // accumulate linear momentum in world space

  // artificial dampening:
  const float fv = 1-b->drag;//powf(1.0f-b->drag, dt);
  for(int k=0;k<3;k++) b->pv[k] *= fv;
  const float fw = 1-b->angular_drag;//powf(1.0f-b->angular_drag, dt);
  for(int k=0;k<3;k++) b->pw[k] *= fw;

  sx_rigid_body_recalculate(b);
}

// integrate position and orientation from accumulated momentums
static inline void sx_rigid_body_move(
    sx_rigid_body_t *body, // update this one
    sx_rigid_body_t *b,    // derivatives are here
    const float dt)
{
  // clear forces
  for(int k=0;k<3;k++) body->force[k] = body->torque[k] = 0.0f;

  // integrate angular velocity -> orientation
  quat_muls(&b->spin, dt);
  quat_add(&body->q, &b->spin);

  // integrate position in world space
  for(int k=0;k<3;k++) body->c[k] += dt * b->v[k];

  // integrate angular momentum in world space
  for(int k=0;k<3;k++) body->pw[k] += dt * b->torque[k];
  // integrate linear momentum in world space
  for(int k=0;k<3;k++) body->pv[k] += dt * b->force[k];

  // recompute dependent quantities:
  sx_rigid_body_recalculate(body);
}

static inline void sx_rigid_body_move_rk4(
    sx_rigid_body_t *body,  // the one to be updated
    sx_rigid_body_t *a,     // we'll destroy the contents of the others
    sx_rigid_body_t *b,
    sx_rigid_body_t *c,
    sx_rigid_body_t *d,
    float dt)            // time step
{
  // clear forces
  for(int k=0;k<3;k++) body->force[k] = body->torque[k] = 0.0f;

  sx_rigid_body_t *arr[4] = {a, b, c, d};
  const float w[4] = {1.0f, 2.0f, 2.0f, 1.0f};

  for(int i=0;i<4;i++)
  {
    // integrate angular velocity -> orientation
    quat_muls(&arr[i]->spin, w[i] * 1.0/6.0f * dt);
    quat_add(&body->q, &arr[i]->spin);

    // integrate position in world space
    for(int k=0;k<3;k++) body->c[k] += 1.0f/6.0f * dt * w[i] * arr[i]->v[k];

    // integrate angular momentum in world space
    for(int k=0;k<3;k++) body->pw[k] += 1.0f/6.0f * dt * w[i] * arr[i]->torque[k];
    // integrate linear momentum in world space
    for(int k=0;k<3;k++) body->pv[k] += 1.0f/6.0f * dt * w[i] * arr[i]->force[k];
  }

  // recompute dependent quantities:
  sx_rigid_body_recalculate(body);
}

