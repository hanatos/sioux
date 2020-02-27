#include "hud.h"
#include "quat.h"
#include "vid.h"
#include "sx.h"
#include "matrix4.h"
#include "camera.h"

static inline int
line(float sx, float sy, float ex, float ey, float *lines)
{
  lines[0] = sx;
  lines[1] = sy;
  lines[2] = ex;
  lines[3] = ey;
  return 2;
}

static inline int
circle(float cx, float cy, float r, float *lines, int n, int dash)
{
  int i = 0;
  for(int k=0;k<=n;k++)
  {
    if(k>1 && !dash)
    { // fill circle, or else it will be dashed
      lines[2*i+0] = lines[2*(i-1)+0];
      lines[2*i+1] = lines[2*(i-1)+1];
      i++;
    }
    lines[2*i+0] = cx + sinf(k*2.0f*M_PI/n)*r * sx.height/(float)sx.width;
    lines[2*i+1] = cy - cosf(k*2.0f*M_PI/n)*r;
    i++;
  }
  if(i & 1)
  { // need even number of vertices for line segments
    lines[2*i+0] = lines[2*(i-n)+0];
    lines[2*i+1] = lines[2*(i-n)+1];
    i++;
  }
  return i;
}

void sx_hud_init(sx_hud_t *hud, const sx_heli_t *heli)
{
  // enough flashing? reset:
  if(hud->flash_num <= sx.time)
  {
    hud->flash_id = s_hud_none;
    hud->flash_beg = hud->flash_end = hud->flash_text_beg = hud->flash_text_end = 0;
  }
  hud->flash_col[0] = 0.2f;
  hud->flash_col[1] = 0.2f;
  hud->flash_col[2] = 0.9f;
  hud->col[0] = 0.3f;
  hud->col[1] = 1.0f;
  hud->col[2] = 0.7f;
  char timestr[256];
  const float *vws = heli->entity->body.v;
  float vos[3] = { // speed in object space
    heli->entity->body.v[0],
    heli->entity->body.v[1],
    heli->entity->body.v[2]};
  quat_transform_inv(&heli->entity->body.q, vos);
  sx_vid_hud_text_clear();
  int i = 0;
  int textpos = 0;

#define HUD_BEG(A)\
  if(hud->flash_id == (A)) hud->flash_beg = i;\
  if(hud->flash_id == (A)) hud->flash_text_beg = textpos;\
  {
#define HUD_END(A)\
  }\
  if(hud->flash_id == (A)) hud->flash_end = i;\
  if(hud->flash_id == (A)) hud->flash_text_end = textpos;

  // mission time
  HUD_BEG(s_hud_time)
  snprintf(timestr, sizeof(timestr), "MT %10.3f", sx.time/1000.0f);
  textpos = sx_vid_hud_text(timestr, textpos, 0.77f, -0.98f, 0, 0);
  HUD_END(s_hud_time)

  HUD_BEG(s_hud_torque) // collective torque
  float cx = -.430f, cy = -.515f, w = 0.013f;
  i += line(cx, cy, cx, cy+0.36f, hud->lines+2*i); // max at 120%
  for(int k=0;k<13;k++)
  {
    float x = cx + w*(k/13.0f - .5f);
    i += line(x, cy, x, cy+0.3f*heli->ctl.collective, hud->lines+2*i);
  }
  i += line(cx-w, cy+0.3f, cx+w, cy+0.3f, hud->lines+2*i); // 100% indicator
  // in ground floating indicator
  i += circle(cx, cy+0.20f, 0.024f, hud->lines+2*i, 10, 0);
  // number box:
  const float bw = 0.031, bh = 0.031;
  cy -= bh;
  i += line(cx-bw, cy-bh, cx+bw, cy-bh, hud->lines+2*i);
  i += line(cx+bw, cy-bh, cx+bw, cy+bh, hud->lines+2*i);
  i += line(cx+bw, cy+bh, cx-bw, cy+bh, hud->lines+2*i);
  i += line(cx-bw, cy+bh, cx-bw, cy-bh, hud->lines+2*i);
  snprintf(timestr, sizeof(timestr), "%03d", (int)(100*heli->ctl.collective));
  textpos = sx_vid_hud_text(timestr, textpos, cx, cy, 1, 1);
  HUD_END(s_hud_torque)

  // ground velocity:
  HUD_BEG(s_hud_groundvel)
  const float cx = 0, cy = -.5f, scale = 4e-3f;
  const float dx = cx - scale*vos[0], dy = cy + scale*vos[2];
  i += line(cx, cy, dx, dy, hud->lines+2*i);
  i += circle(dx, dy, 0.022f, hud->lines+2*i, 13, 0);
  float speed = ms2knots(sqrtf(vws[0]*vws[0]+vws[2]*vws[2])); // in knots
  snprintf(timestr, sizeof(timestr), "SPEED %3.0f", speed);
  textpos = sx_vid_hud_text(timestr, textpos, 0.77f, 0.10f, 0, 0);
  HUD_END(s_hud_groundvel)

  // altitude above ground
  HUD_BEG(s_hud_altitude)
  const float aog = sx_heli_alt_above_ground(heli);
  const float cx = -.54f, cy = -.3f;
  i += line(cx, cy, cx, cy+0.6f, hud->lines+2*i);

  // compute relative vertical speed in world space
  const float vas = vws[1];

  // first put everything to altitude above ground
  float ht = 0.001f * aog;
  i += line(cx+0.01f, cy+ht, cx-0.02f, cy+ht+0.001f, hud->lines+2*i);
  i += line(cx+0.01f, cy+ht, cx-0.02f, cy+ht-0.001f, hud->lines+2*i);

  // now move relative vertical air speed up or down
  ht += 0.010f * vas;
  i += line(cx+0.01f, cy+ht, cx-0.02f, cy+ht+0.01f, hud->lines+2*i);
  i += line(cx+0.01f, cy+ht, cx-0.02f, cy+ht-0.01f, hud->lines+2*i);
  snprintf(timestr, sizeof(timestr), "ALT %3d", (int)(aog));
  textpos = sx_vid_hud_text(timestr, textpos, cx, cy+0.63f, 1, 0);
  HUD_END(s_hud_altitude)

  HUD_BEG(s_hud_compass)
  float head[3] = {0.0f, 0.0f, 1.0f}; // world space heading
  // quat_transform(&heli->entity->body.q, head);
  quat_transform(&sx.cam.q, head);
  const float heading = atan2f(-head[0], head[2]);
  // text is in NDC
  snprintf(timestr, sizeof(timestr), "%03d", ((int)(360+180.0f/M_PI*heading))%360);
  textpos = sx_vid_hud_text(timestr, textpos, 0.0f, 0.9f, 1, 0);
  // find north (=0 angle) to the left of our screen in NDC coordinates:
  float north = -heading * 2.0f / sx.cam.hfov;
  const float thirty = 2.0f/sx.cam.hfov * M_PI/6.0f;
  const float five   = 2.0f/sx.cam.hfov * M_PI/36.0f;
  if(north+12*thirty <  1.0f) north += 12 * thirty;
  if(north           > -1.0f) north -= 12 * thirty;
  const char *compass_text[] = {
    "N", "030", "060", "E", "120", "150",
    "S", "210", "240", "W", "300", "330"};
  for(int k=0;k<24;k++)
    textpos = sx_vid_hud_text(compass_text[k%12], textpos,
        north + k * thirty, 0.80, 1, 0);
  for(int k=0;k<288;k+=2)
  {
    int tick = k/2;
    hud->lines[2*i+0] = north + tick * five;
    hud->lines[2*i+1] = (tick & 1) ? 0.85 : 0.87;
    hud->lines[2*i+2] = north + tick * five;
    hud->lines[2*i+3] = 0.90;
    i += 2;
  }
  HUD_END(s_hud_compass)

  HUD_BEG(s_hud_waypoint)
  for(int p=0;p<2;p++)
  {
    uint32_t curr_wp = sx.world.player_wp + p;
    float wp[4] = { // waypoint in worldspace
      sx.mission.waypoint[0][curr_wp][0],
      0.0f,
      sx.mission.waypoint[0][curr_wp][1],
      1.0f,
    };
    wp[1] = sx_world_get_height(wp);

#if 1
    float MVP[16], MV[16], wph[4] = {0, 0, 0, 1};
    quat_t nop;
    quat_init(&nop, 0, 1, 0, 0);
    sx_vid_compute_mvp(MVP, MV, -1, &nop, wp, &sx.cam, 0, 0);
    mat4_mulv(MVP, wph, wp);
    if(wp[2] < 0) continue; // discard behind camera
    for(int k=0;k<3;k++) wp[k] /= wp[3];
    float x = wp[0];
    float y = wp[1];
#else
    // convert to camera space:
    for(int k=0;k<3;k++) wp[k] -= sx.cam.x[k];
    quat_transform_inv(&sx.cam.q, wp);
    // intersect with spherical coordinates:
    const float angley = asinf(wp[1] / sqrtf(wp[0]*wp[0] + wp[2]*wp[2]));
    const float anglex = -atan2f(wp[2], wp[0])+M_PI/2.0f;
    float x = -2.f * anglex / sx.cam.hfov;
    float y =  2.f * angley / sx.cam.vfov;
#endif

    // int i = waypoint_begin;
    hud->lines[2*i+0] = x;
    hud->lines[2*i+1] = y;
    hud->lines[2*i+2] = x;
    hud->lines[2*i+3] = y+0.20f;
    i += 2;
    float cx = x, cy = y+0.25f;
    char text[3] = {'A', '1', 0};
    text[1] = '1' + curr_wp;
    textpos = sx_vid_hud_text(text, textpos, cx, cy, 1, 1);
    i += circle(cx, cy, 0.05f, hud->lines+2*i, 13, p);
  }
  HUD_END(s_hud_waypoint)

  HUD_BEG(s_hud_center)
  // where does the nose of the helicopter point to wrt artificial horizon?
  // coordinates are NDC [-1,1]^2
  { // first draw artificial horizon lines by drawing worldspace directions
    sx_entity_t *ent = sx.world.entity + sx.world.player_entity;
    float MVP[16], MV[16], x0[4], x1[4];
    // find quaternion that only encodes yaw
    float front[3] = {0, 0, 1};
    quat_transform(&ent->body.q, front); // to world space
    float angle = atan2f(front[0], front[2]);
    quat_t q1;
    quat_init_angle(&q1, angle, 0, 1, 0);
    sx_vid_compute_mvp(MVP, MV, -1, &q1, sx.cam.x, &sx.cam, 0, 0);
    const float wd[] = {4.f, 1.f, 1.f, 2.f, 1.f, 1.f, 2.f, 1.f, .5f};
    for(int h=-8;h<=8;h++)
    {
      float a = M_PI*h*10.0/180.0f;
      float p0[4] = {-wd[abs(h)], 10*sinf(a), 10*cosf(a), 1};
      float p1[4] = { wd[abs(h)], 10*sinf(a), 10*cosf(a), 1};
      mat4_mulv(MVP, p0, x0);
      mat4_mulv(MVP, p1, x1);
      if(x0[2] < 0 || x1[2] < 0) continue; // discard behind camera
      for(int k=0;k<3;k++) x0[k] /= x0[3];
      for(int k=0;k<3;k++) x1[k] /= x1[3];
      hud->lines[2*i+0] = x0[0];
      hud->lines[2*i+1] = x0[1];
      hud->lines[2*i+2] = x1[0];
      hud->lines[2*i+3] = x1[1];
      i+=2;
    }
  }
  { // then draw center where helicopter nose is pointing to
    float MVP[16], MV[16], ph[4] = {0, 0, 10, 1}, p[4];
    sx_entity_t *ent = sx.world.entity + sx.world.player_entity;
    sx_vid_compute_mvp(MVP, MV, -1, &ent->body.q, sx.cam.x, &sx.cam, 0, 0);
    mat4_mulv(MVP, ph, p);
    const float cursor[] = {
      -0.1f, 0.f, -0.04f, 0.f, -0.04f, 0.f, -0.04f, -0.04f,
       0.1f, 0.f,  0.04f, 0.f,  0.04f, 0.f,  0.04f, -0.04f};
    int size = sizeof(cursor)/sizeof(cursor[0]);
    if(p[2] > 0) // discard behind camera
    {
      for(int k=0;k<3;k++) p[k] /= p[3];
      for(int k=0;k<size;k+=2)
      {
        hud->lines[2*i+k  ] = cursor[k]   + p[0];
        hud->lines[2*i+k+1] = cursor[k+1] + p[1];
      }
      i += size/2;
    }
  }
  HUD_END(s_hud_center)
  hud->num_lines = i;
#undef HUD_BEG
#undef HUD_END
}

void sx_hud_flash(sx_hud_t *hud, uint32_t id, uint32_t times)
{
  hud->flash_id = id;
  hud->flash_num = sx.time + times * 1000.0f;
}
