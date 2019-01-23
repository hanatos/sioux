#pragma once
#include "quat.h"

typedef enum sx_camera_mode_t
{
  s_cam_inside_cockpit = 0,
  s_cam_inside_no_cockpit,
  s_cam_left,
  s_cam_right,
  s_cam_homing,
  s_cam_flyby,
}
sx_camera_mode_t;

typedef struct sx_camera_t
{
  // current position
  quat_t q;    // object to world orientation quaternion
  float x[3];

  float hfov; // horizontal field of view in radians
  float vfov; // vertical   field of view in radians

  // previous position
  quat_t prev_q;
  float prev_x[3];

  // goto position
  quat_t target_q;
  float target_x[3];
  float target_rate;

  float angle_down, angle_right;

  sx_camera_mode_t mode;
}
sx_camera_t;

static inline void sx_camera_init(sx_camera_t *cam)
{
  memset(cam, 0, sizeof(*cam));
}

static inline void sx_camera_move(sx_camera_t *cam, const float dt)
{
  for(int k=0;k<3;k++)
    cam->prev_x[k] = cam->x[k];
  cam->prev_q = cam->q;

  const float r = 1.0f-powf(1.0f-cam->target_rate, dt/0.02f);
  for(int k=0;k<3;k++)
    cam->x[k] = cam->prev_x[k] * (1.0f-r) + cam->target_x[k] * r;
  quat_slerp(&cam->prev_q, &cam->target_q, r, &cam->q);
  quat_normalise(&cam->q);
}

static inline void sx_camera_target(
    sx_camera_t *cam, // camera to modify
    const float *x,   // position of target
    const quat_t *q,  // orientation (object to world) of target
    const float *off, // offset in world space
    float rate)       // convergence rate, higher is faster (<=1.0)
{
  for(int k=0;k<3;k++) cam->target_x[k] = x[k] + off[k];
  cam->target_q = *q;
  cam->target_rate = rate;
}

static inline void sx_camera_lookat(
    sx_camera_t *cam,
    const float *lookat, // world space look at coordinates
    float rate)
{
  quat_t q;
  float to[3] = {lookat[0] - cam->x[0], lookat[1] - cam->x[1], lookat[2] - cam->x[2]};
  normalise(to);
  float roll = 0.0f;
  float yaw = atan2f(to[0], to[2]);
  float pitch = -asinf(CLAMP(to[1], -1.0f, 1.0f));
  quat_from_euler(&q, roll, pitch, yaw);
  quat_normalise(&q);

  cam->target_q = q;
  cam->target_rate = rate;
}
