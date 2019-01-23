#include "hud.h"
#include "quat.h"
#include "vid.h"
#include "sx.h"

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
  // text is in NDC, not svg size
  // find next north marker to the left of our screen in NDC coordinates:
  float north = -heading * 2.0f / sx.cam.hfov;
  snprintf(timestr, sizeof(timestr), "%03d", ((int)(360+180.0f/M_PI*heading))%360);
  textpos = sx_vid_hud_text(timestr, textpos, 0.0f, 0.9f, 1, 0);
  const float thirty = 2.0f/sx.cam.hfov * M_PI/6.0f;
  const float five   = 2.0f/sx.cam.hfov * M_PI/36.0f;
  if(north+12*thirty <  1.0f) north += 2.0f*M_PI * 2.0f / sx.cam.hfov;
  if(north           > -1.0f) north -= 2.0f*M_PI * 2.0f / sx.cam.hfov;
  const char *compass_text[] = { "N", "030", "060", "E", "120", "150", "S", "210", "240", "W", "300", "330"};
  for(int k=0;k<24;k++)
    textpos = sx_vid_hud_text(compass_text[k%12], textpos,
        north + k * thirty, 0.80, 1, 0);
  for(int k=0;k<144;k+=2)
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
    float wp[3] = { // waypoint in worldspace
      sx.mission.waypoint[0][curr_wp][0],
      0.0f,
      sx.mission.waypoint[0][curr_wp][1],
    };
    wp[1] = sx_world_get_height(wp);
    // convert to camera space:
    for(int k=0;k<3;k++) wp[k] -= sx.cam.x[k];
    quat_transform_inv(&sx.cam.q, wp);
    // intersect with spherical coordinates:
    const float angley = asinf(wp[1] / sqrtf(wp[0]*wp[0] + wp[2]*wp[2]));
    const float anglex = -atan2f(wp[2], wp[0])+M_PI/2.0f;
    float x = -2.f * anglex / sx.cam.hfov;
    float y =  2.f * angley / sx.cam.vfov;

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
  hud->num_lines = i;
#undef HUD_BEG
#undef HUD_END
}

void sx_hud_flash(sx_hud_t *hud, uint32_t id, uint32_t times)
{
  hud->flash_id = id;
  hud->flash_num = sx.time + times * 1000.0f;
}
