#include "physics/rigidbody.h"
#include "world.h"
#include <stdlib.h>
#include <string.h>

static void
update_forces(void *vp, sx_rigid_body_t *b)
{
  // TODO: constant acceleration forward
  quat_t w2o = b->q; // q is object -> world
  w2o.w = -w2o.w; // invert rotation
  sx_actuator_t a = {{0},{0}};
  a.f[0] = 1.0f; // 1 N
  a.r[2] = 1.0f;
  quat_transform(&w2o, a.f);

  sx_rigid_body_apply_force(b, &a);
  // float torque[3] = {0.0f, 1.f, 0.0f};
  // quat_transform(&w2o, torque);
  // sx_rigid_body_apply_torque(b, torque);
}

static void
move(
    sx_entity_t *e,
    const float dt)
{
  if(!e->move_data) return;
  if(!e->move.update_forces) return;

  for(int k=0;k<3;k++)
    e->prev_x[k] = e->body.c[k];
  e->prev_q = e->body.q;

#if 0 // euler. wow this is bad.
  e->move.update_forces(e->move_data, &e->body);
  sx_rigid_body_recalculate(&e->body);
  sx_rigid_body_move(&e->body, dt);
return;
#endif

  // runge kutta 4:
  // (a)
  sx_rigid_body_t a = e->body;
  e->move.update_forces(e->move_data, &a);
  // (b)
  sx_rigid_body_t b = e->body;
  // integrate derivatives (a) onto c, pv, q, pw (in this order) * dt/2
  sx_rigid_body_eval(&b, &a, dt*.5f);
  e->move.update_forces(e->move_data, &b);
  // (c)
  sx_rigid_body_t c = e->body;
  // integrate deriv (b) onto c pv q pw * dt/2
  sx_rigid_body_eval(&c, &b, dt*.5f);
  e->move.update_forces(e->move_data, &c);
  // (d)
  sx_rigid_body_t d = e->body;
  sx_rigid_body_eval(&d, &c, dt);
  e->move.update_forces(e->move_data, &d);
  // now update p pv q pw with 1/6 dt 1 2 2 1 rule.
  sx_rigid_body_move_rk4(&e->body, &a, &b, &c, &d, dt);
  // convert from meters to whatever this worldspace is (height seems to work out at feet/5.0f? length more like 3.0/5.0)
  // for(int k=0;k<3;k++)
    // e->body.c[k] = e->prev_x[k] + 5.0f*(e->body.c[k] - e->prev_x[k]);
}

int main(int argc, char *argv[])
{
  sx_entity_t e;
  memset(&e, 0, sizeof(e));
  e.move_data = (void*)1;
  e.move.update_forces = &update_forces;

  e.body.m = 1.0f; // 1 kg
  e.body.invI[0] = e.body.invI[4] = e.body.invI[8] = 1.0f; // 1/mass on diagonal
  quat_init_angle(&e.body.q, 0, 0, 0, 1); // identity quaternion

  int num = 100;
  float t = 1.0f;
  for(int i=0;i<=num;i++)
  {
    float dt = t / num;
    float vw[3] = {e.body.v[0], e.body.v[1], e.body.v[2]};
    quat_transform(&e.body.q, vw);

    fprintf(stderr, "t = %0.2f x %.2f %.2f %.2f v %.2f %.2f %.2f o %.2f %.2f %.2f\n",
        dt * i,
        e.body.c[0], e.body.c[1], e.body.c[2],
        vw[0], vw[1], vw[2],
        e.body.q.x[0], e.body.q.x[1], e.body.q.x[2]);
    move(&e, dt);
  }
  exit(0);
}
