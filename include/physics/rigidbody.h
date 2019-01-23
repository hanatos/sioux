#pragma once

#include "quat.h"
#include "matrix3.h"
#include <assert.h>

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
  float pw[3];     // angular momentum in object space

  // constant quantities:
  float invI[9];   // inverse inertia tensor in object space
  float m;         // total mass

  // derived quantities:
  float v[3];      // velocity in world space [m/s]
  float w[3];      // angular velocity in object space [rad/s]
  quat_t spin;     // world space spin (dq/dt)

  // collected forces:
  float torque[3]; // collected rotational force (= r x f)
  float force[3];  // collected linear force (both in object space)

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
  // XXX coordinate system madness? can be solved more elegantly i suppose?
  //b->torque[0] += torque[0]; // XXX - here looks correct
  //b->torque[1] += torque[1];
  //b->torque[2] += torque[2];
}

// accumulate linear force and torque
static inline void sx_rigid_body_apply_force(
    sx_rigid_body_t *b,
    const sx_actuator_t *act)
{
  // collect actuator force, linear part:
  for(int k=0;k<3;k++) b->force[k] += act->f[k];

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
  mat3_mulv(b->invI, b->pw, b->w); // compute angular velocity in object space

#if 0
  // the following holds if w is in world space:
  // spin = dt * 0.5 * w * q;
  float ww[3] = {b->w[0], b->w[1], b->w[2]};
  quat_transform(&b->q, ww); // convert w to world space
  quat_t wq;
  quat_init(&wq, 0.0f, ww[0], ww[1], ww[2]);
  quat_mul(&wq, &b->q, &b->spin);
  quat_muls(&b->spin, 0.5f);
#else // use w in object coordinates:
  // spin = qd * dt = 0.5 * q * w
  quat_t wq;
  quat_init(&wq, 0.0f, b->w[0], b->w[1], b->w[2]);
  quat_mul(&b->q, &wq, &b->spin);
  quat_muls(&b->spin, 0.5f);
#endif
}

static inline void sx_rigid_body_eval(
    sx_rigid_body_t *b,            // rigid body to update
    const sx_rigid_body_t *deriv,  // the one with the derivatives to use
    float dt)                      // time step
{
  // float pv_ws[3] = {b->pv[0], b->pv[1], b->pv[1]};
  // quat_transform(&b->q, pv_ws); // to world space, we want this to be constant
  // integrate angular velocity -> orientation
  quat_t spin_dt = deriv->spin;
  quat_muls(&spin_dt, dt);

  quat_add(&b->q, &spin_dt);
  quat_normalise(&b->q);

  // float vw[3] = {deriv->v[0], deriv->v[1], deriv->v[2]}; // velocity in world space
  // quat_transform(&deriv->q, vw);
  // quat_transform(&b->q, vw);
  // for(int k=0;k<3;k++) b->c[k] += dt * vw[k]; // integrate position in world space
  for(int k=0;k<3;k++) b->c[k] += dt * deriv->v[k]; // integrate position in world space

  // artificial dampening:
  const float fv = powf(1.0f-b->drag, dt);
  b->pv[0] *= fv;
  b->pv[1] *= fv;
  b->pv[2] *= fv;
  const float fw = powf(1.0f-b->angular_drag, dt);
  b->pw[0] *= fw;
  b->pw[1] *= fw;
  b->pw[2] *= fw;

  float forcew[3] = {deriv->force[0], deriv->force[1], deriv->force[2]}; // force in world space
  quat_transform(&b->q, forcew);
  // quat_transform_inv(&b->q, pv_ws); // to object space again
  for(int k=0;k<3;k++) b->pw[k] += deriv->torque[k] * dt; // integrate angular momentum
  // b->pw[0] += deriv->torque[0] * dt; // integrate angular momentum
  // b->pw[1] += deriv->torque[1] * dt; // integrate angular momentum
  // b->pw[2] += deriv->torque[2] * dt; // integrate angular momentum
  // for(int k=0;k<3;k++) b->pv[k] += deriv->force [k] * dt; // accumulate momentum
  for(int k=0;k<3;k++) b->pv[k] += forcew[k] * dt; // accumulate momentum

  sx_rigid_body_recalculate(b);
}

// integrate position and orientation from accumulated momentums
static inline void sx_rigid_body_move(
    sx_rigid_body_t *b,
    const float dt)
{
  // euler:
  for(int k=0;k<3;k++) b->pw[k] += b->torque[k] * dt; // integrate angular momentum
  float forcew[3] = {b->force[0], b->force[1], b->force[2]}; // force in world space
  quat_transform(&b->q, forcew);
  for(int k=0;k<3;k++) b->pv[k] += forcew[k] * dt; // accumulate momentum
  // for(int k=0;k<3;k++) b->pv[k] += b->force [k] * dt; // accumulate momentum

  for(int k=0;k<3;k++) b->v[k] = b->pv[k] / b->m; // integrate velocity in world space
  mat3_mulv(b->invI, b->pw, b->w); // compute angular velocity in object space

  // integrate angular velocity -> orientation
#if 0
  // the following holds if w is in world space:
  // q += dt * 0.5 * w * q;
  float ww[3] = {b->w[0], b->w[1], b->w[2]};
  quat_transform(&b->q, ww); // convert w to world space
  quat_t wq;
  quat_init(&wq, 0.0f, ww[0], ww[1], ww[2]);
  quat_mul(&wq, &b->q, &b->spin);
#else
  // spin = qd * dt = 0.5 * q * w
  quat_t wq;
  quat_init(&wq, 0.0f, b->w[0], b->w[1], b->w[2]);
  quat_mul(&b->q, &wq, &b->spin);
#endif
  quat_muls(&b->spin, 0.5f*dt);
  quat_add(&b->q, &b->spin);
  // b->q.x[0] -= b->spin.x[0];
  // b->q.x[1] += b->spin.x[1];
  // b->q.x[2] += b->spin.x[2];
  // b->q.w += b->spin.w;
  quat_normalise(&b->q);

  // float vw[3] = {b->v[0], b->v[1], b->v[2]}; // velocity in world space
  // quat_transform(&b->q, vw);
  // for(int k=0;k<3;k++) b->c[k] += dt * vw[k]; // integrate position in world space
  for(int k=0;k<3;k++) b->c[k] += dt * b->v[k]; // integrate position in world space
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

    // velocity in world space
    // float vw[3] = {arr[i]->v[0], arr[i]->v[1], arr[i]->v[2]};
    // quat_transform(&arr[i]->q, vw);
    // for(int k=0;k<3;k++) body->c[k] += 1.0f/6.0f * dt * w[i] * vw[k];
    // integrate position in world space
    for(int k=0;k<3;k++) body->c[k] += 1.0f/6.0f * dt * w[i] * arr[i]->v[k];

    // integrate angular momentum
    for(int k=0;k<3;k++) body->pw[k] += 1.0f/6.0f * dt * w[i] * arr[i]->torque[k];
    // integrate linear momentum
    float forcew[3] = {arr[i]->force[0], arr[i]->force[1], arr[i]->force[2]}; // force in world space
    quat_transform(&b->q, forcew);
    for(int k=0;k<3;k++) body->pv[k] += 1.0f/6.0f * dt * w[i] * forcew[k];//arr[i]->force[k];
  }

  // recompute dependent quantities:
  sx_rigid_body_recalculate(body);
}

