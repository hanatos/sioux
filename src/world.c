#include "sx.h"
#include "physics/rigidbody.h"
#include "physics/heli.h"
#include "physics/accel.h"
#include "physics/obb_obb.h"
#include "plot/common.h"
#include "pngio.h"
#include "gameplay.h"

#include <stdint.h>

static void
compute_one_aabb(
    uint64_t  eid,
    float    *aabb)
{
  sx_entity_t *e = sx.world.entity + eid;
  const float *mb = sx.assets.object[e->objectid].aabb;
  aabb[0] = aabb[1] = aabb[2] =  FLT_MAX;
  aabb[3] = aabb[4] = aabb[5] = -FLT_MAX;
  for(int j=0;j<8;j++)
  { // transform the 8 corners and bound them
    float x[3];
    for(int k=0;k<3;k++)
      x[k] = (j&(1<<k)?1.0:-1.0)*(mb[3+k] - mb[k])*.5f;
    quat_transform(&e->body.q, x);
    for(int k=0;k<3;k++) x[k] += e->body.c[k];
    aabb[0] = MIN(aabb[0], x[0]);
    aabb[1] = MIN(aabb[1], x[1]);
    aabb[2] = MIN(aabb[2], x[2]);
    aabb[3] = MAX(aabb[0], x[0]);
    aabb[4] = MAX(aabb[4], x[1]);
    aabb[5] = MAX(aabb[5], x[2]);
  }
}

static void
compute_aabb(
    uint64_t  beg,
    uint64_t  end,
    float    *aabb)
{
  // fill bounding boxes of all objects in [beg, end)
  // one object in the bvh will be one entity. the primitives
  // will be the obb of the individual geo parts
  for(uint64_t i=beg;i<end;i++)
    compute_one_aabb(i, aabb+6*i);
}

void sx_world_init()
{
  // 2000 entities is enough for everyone, right?
  // if not the bvh should probably reallocate internally.
  sx.world.bvh = accel_init(2000, compute_aabb);
}

void sx_world_cleanup()
{
  accel_cleanup(sx.world.bvh);
}

// apply one step of runge-kutta 4 integration
// to the equations of movement of the given entity
static void
sx_world_move_entity(
    sx_entity_t *e,
    const float dt)
{
  if(!e->move_data) return;
  if(!e->move.update_forces) return;

  if(sx.assets.object[e->objectid].collidable)
  {
    uint64_t coll_ent[10];
    uint64_t coll_cnt = 10;
    float aabb[6]; // world space aabb
    const int eid = e - sx.world.entity;
    compute_one_aabb(eid, aabb);
    coll_cnt = accel_collide(sx.world.bvh, aabb, eid, coll_ent, coll_cnt);
    for(int i=0;i<coll_cnt;i++)
    {
      sx_entity_t *e2 = sx.world.entity+coll_ent[i];
      if(sx.assets.object[e2->objectid].projectile)
        if(e->move.damage)
          e->move.damage(e, e2, 0.0f);
    }
  }

  for(int k=0;k<3;k++)
    e->prev_x[k] = e->body.c[k];
  e->prev_q = e->body.q;

  // runge kutta 4:
  // (a)
  sx_rigid_body_t a = e->body;
  e->move.update_forces(e, &a);
  // (b)
  sx_rigid_body_t b = e->body;
  // integrate derivatives (a) onto c, pv, q, pw (in this order) * dt/2
  sx_rigid_body_eval(&b, &a, dt*.5f);
  e->move.update_forces(e, &b);
  // (c)
  sx_rigid_body_t c = e->body;
  // integrate deriv (b) onto c pv q pw * dt/2
  sx_rigid_body_eval(&c, &b, dt*.5f);
  e->move.update_forces(e, &c);
  // (d)
  sx_rigid_body_t d = e->body;
  sx_rigid_body_eval(&d, &c, dt);
  e->move.update_forces(e, &d);
  // now update p pv q pw with 1/6 dt 1 2 2 1 rule.
  sx_rigid_body_move_rk4(&e->body, &a, &b, &c, &d, dt);
}

static int
sx_world_collide_terrain(
    sx_entity_t *e)
{
  if(!e->move_data) return 0;
  if(!e->move.update_forces) return 0;

  // check a few points of the aabb-ellipsoid against collision with the ground
  // TODO: can we have rotors, too? or do we delegate that to the individual objects?
  // checking rotors to ground is arguably not a major concern.

  // TODO: damage relevant points? define in interface?
  // TODO: could also have points for gear/docking etc

  // TODO: maybe need to delegate to individual move routines
  // hard clamp and set external forces to 0?

  // fprintf(stderr, "bam! %g %g %g vv %g\n", body->c[1], groundlevel, ht, body->pv[1]);
  if(e->move.damage) e->move.damage(e, 0, 0);

  return 1;
}

// TODO: collision detection and interaction between objects
void sx_world_move(const uint32_t dt_milli)
{
  sx_vid_clear_debug_line();
  const float dt = dt_milli / 1000.0f; // delta t in seconds

  // build collision detection bvh every time because why not
  accel_set_prim_cnt(sx.world.bvh, sx.world.num_entities);
  accel_build_async(sx.world.bvh);
  // could do expensive GPU stuff here..
  accel_build_wait(sx.world.bvh);

  // move entities based on the forces we collected during the last iteration
  // TODO: do in our thread pool, faster (openmp needs 2ms thread startup time)
#ifdef _OPENMP
#pragma omp parallel for
#endif
  for(int i=0;i<sx.world.num_entities;i++)
  {
    sx_world_move_entity(sx.world.entity + i, dt);
    // collide everybody with the ground plane. that's easy because the ground doesn't move:
    sx_world_collide_terrain(sx.world.entity + i);
  }

  // for all objects
  //   get damage points and check against bounding boxes of all other potentially overlapping
  //   objects (use hashed grid from above)

  // update camera after everything else, it may be homing/locked to objects:
  if(sx.cam.mode == s_cam_homing)
  {
    // x left; y up; z front;
    // float off[3] = {0, 2.3, -18};
    float off[3] = {3, 2.3, -18};
    // float off[3] = {-5, -.5, 10};
    const float *pos = sx.world.entity[sx.world.player_entity].body.c;
    const quat_t *q = &sx.world.entity[sx.world.player_entity].body.q;
    float wso[3] = {off[0], off[1], off[2]};
    quat_transform(q, wso); // convert object to world space
    for(int k=0;k<3;k++) off[k] = pos[k] + wso[k];
    float minh = 1.0f + sx_world_get_height(off);
    if(wso[1] + pos[1] < minh) wso[1] = minh - pos[1];
    sx_camera_target(&sx.cam, pos, q, wso, 0.03f, 0.03f);
    sx_camera_lookat(&sx.cam, pos, 0.03f, 0.03f);
  }
  else if(sx.cam.mode == s_cam_rotate)
  {
    // x left; y up; z front;
    float off[3] = {3, 2.0, -15};
    quat_t rot;
    quat_init_angle(&rot, sx.time*0.0001, 0, 1, 0);
    quat_transform(&rot, off);
    const float *pos = sx.world.entity[sx.world.player_entity].body.c;
    const quat_t *q = &sx.world.entity[sx.world.player_entity].body.q;
    float wso[3] = {off[0], off[1], off[2]};
    quat_transform(q, wso); // convert object to world space
    for(int k=0;k<3;k++) off[k] = pos[k] + wso[k];
    float minh = 1.0f + sx_world_get_height(off);
    if(wso[1] + pos[1] < minh) wso[1] = minh - pos[1];
    sx_camera_target(&sx.cam, pos, q, wso, 0.03f, 0.03f);
    sx_camera_lookat(&sx.cam, pos, 0.03f, 0.03f);
  }
  else if(sx.cam.mode == s_cam_inside_cockpit || sx.cam.mode == s_cam_inside_no_cockpit)
  {
    const float off[3] = {0, 1.40f, 2.9f}; // front pilot's head
    const float *pos = sx.world.entity[sx.world.player_entity].body.c;
    const quat_t *q = &sx.world.entity[sx.world.player_entity].body.q;
    quat_t tmp = *q; // look down a bit
    quat_t down; quat_init_angle(&down, 0.15f+sx.cam.angle_down, 1.0f, 0.0f, 0.0f);
    quat_t right; quat_init_angle(&right, -sx.cam.angle_right, 0.0f, 1.0f, 0.0f);
    quat_mul(q, &down, &tmp);
    quat_mul(&tmp, &right, &down);
    float wso[3] = {off[0], off[1], off[2]};
    quat_transform(q, wso); // convert object to world space
    sx_camera_target(&sx.cam, pos, &down, wso, 0.9f, 0.9f);
  }
  else if(sx.cam.mode == s_cam_left || sx.cam.mode == s_cam_right)
  {
    const float off[3] = {0, 1.40f, 2.9f}; // front pilot's head
    const float *pos = sx.world.entity[sx.world.player_entity].body.c;
    const quat_t *q = &sx.world.entity[sx.world.player_entity].body.q;
    float wso[3] = {off[0], off[1], off[2]};
    float target[3] = {
      ((sx.cam.mode == s_cam_left) ? 1.0 : -1.0),
      -0.2f, 0.15f};
    quat_transform(q, wso); // convert object to world space
    quat_transform(q, target);
    for(int k=0;k<3;k++) target[k] += sx.cam.x[k];
    sx_camera_target(&sx.cam, pos, q, wso, 0.9f, 0.03f);
    sx_camera_lookat(&sx.cam, target, 0.9f, 0.03f);
  }

  sx_camera_move(&sx.cam, dt);
}

void sx_world_render_entity(uint32_t ei)
{
  if(ei < 0 || ei >= sx.world.num_entities) return;
  uint32_t oi = sx.world.entity[ei].objectid;
  if(oi < 0 || oi >= sx.assets.num_objects) return;

  const sx_entity_t *ent = sx.world.entity+ei;
  // LOD: discard stuff > 2km
  float d[3], md = 2000;
  for(int k=0;k<3;k++) d[k] = ent->body.c[k] - sx.cam.x[k];
  if(dot(d, d) > md*md) return;

  if(//sx.cam.mode == s_cam_inside_cockpit ||
     sx.cam.mode == s_cam_inside_no_cockpit)
    if(ei == sx.world.player_entity) return;

  if(ent->plot.plot)
    ent->plot.plot(ei);
}

float
sx_world_get_height(
    const float *p)
{
  const float scale = 1.0f/(3.0f*0.3048f*2048.0f);
  int coord[2];
  coord[0] = fmodf(sx.world.terrain_wd * 100 + sx.world.terrain_wd * p[0] * scale, sx.world.terrain_wd);
  coord[1] = fmodf(sx.world.terrain_ht * 100 + sx.world.terrain_ht * p[2] * scale, sx.world.terrain_ht);
  return sx.world.terrain_low + sx.world.terrain_high/256.0f * (sx.world.terrain[
    (sx.world.terrain_wd * (sx.world.terrain_ht - 1 - coord[1]) + sx.world.terrain_wd - 1 - coord[0])]);
}

void
sx_world_get_normal(const float *p, float *n)
{
  const float scale = 1.0f/(3.0f*0.3048f*2048.0f);
  int coord[2];
  coord[0] = fmodf(sx.world.terrain_wd * 100 + sx.world.terrain_wd * p[0] * scale, sx.world.terrain_wd);
  coord[1] = fmodf(sx.world.terrain_ht * 100 + sx.world.terrain_ht * p[2] * scale, sx.world.terrain_ht);
  int c[2];
  c[0] = fmodf(coord[0] - 1, sx.world.terrain_wd);
  c[1] = coord[1];
  float h0 = sx.world.terrain_low + sx.world.terrain_high/256.0f * (
      sx.world.terrain[
    (sx.world.terrain_wd * (sx.world.terrain_ht - 1 - c[1]) +
     sx.world.terrain_wd - 1 - c[0])]);
  c[0] = fmodf(coord[0] + 1, sx.world.terrain_wd);
  c[1] = coord[1];
  float h1 = sx.world.terrain_low + sx.world.terrain_high/256.0f * (
      sx.world.terrain[
    (sx.world.terrain_wd * (sx.world.terrain_ht - 1 - c[1]) +
     sx.world.terrain_wd - 1 - c[0])]);
  c[0] = coord[0];
  c[1] = fmodf(coord[1] - 1, sx.world.terrain_ht);
  float h2 = sx.world.terrain_low + sx.world.terrain_high/256.0f * (
      sx.world.terrain[
    (sx.world.terrain_wd * (sx.world.terrain_ht - 1 - c[1]) +
     sx.world.terrain_wd - 1 - c[0])]);
  c[0] = coord[0];
  c[1] = fmodf(coord[1] + 1, sx.world.terrain_ht);
  float h3 = sx.world.terrain_low + sx.world.terrain_high/256.0f * (
      sx.world.terrain[
    (sx.world.terrain_wd * (sx.world.terrain_ht - 1 - c[1]) +
     sx.world.terrain_wd - 1 - c[0])]);

  float u[] = {2.0f, h1 - h0, 0.0f};
  float v[] = {0.0f, h3 - h2, 2.0f};
  cross(v, u, n);
  normalise(n);
}

void
sx_world_remove_entity(
    uint32_t eid)
{
  sx.world.num_entities--;
  sx.world.entity[eid] = sx.world.entity[sx.world.num_entities];
  memset(sx.world.entity + sx.world.num_entities, 0, sizeof(sx_entity_t));
}

uint32_t
sx_world_add_entity(
    uint32_t objectid,   // 3d model to link to
    const float *p,      // position
    const quat_t *q,     // orientation
    char id,             // mission relevant id
    uint8_t camp)        // good guy, bad guy, neutral
{
  if(sx.world.num_entities >= sizeof(sx.world.entity)/sizeof(sx.world.entity[0]))
    return -1;
  int eid = sx.world.num_entities++;
  memset(sx.world.entity + eid, 0, sizeof(sx_entity_t));

  // geometry stuff:
  sx.world.entity[eid].objectid = objectid;
  sx.world.entity[eid].id = id;
  sx.world.entity[eid].camp = camp;
  sx.world.entity[eid].hitpoints = sx.assets.object[objectid].hitpoints;
  sx.world.entity[eid].birth_time = sx.time;

  // init location and orientation
  for(int k=0;k<3;k++) sx.world.entity[eid].prev_x[k] = p[k];
  sx.world.entity[eid].prev_q = *q;
  for(int k=0;k<3;k++) sx.world.entity[eid].body.c[k] = p[k];
  sx.world.entity[eid].body.q = *q;

  // put the object to the ground.
  // the move routine should take care of more complicated things,
  // e.g. helicopters 0 is at the rotor they need to move up.
  // (when using the aabb, note that the objects have a z-up system)
  float ht = sx_world_get_height(p);
  sx.world.entity[eid].prev_x[1] = ht;
  sx.world.entity[eid].body.c[1] = ht;

  const float rho = 50.0f; // mass density

  // derived quantities:
  // TODO: this the good bounding box?
  // const float *aabb = sx.assets.object[objectid].geo_aabb[0]; // just the body
  const float w = 2.0f, l = 2.0f, h = 3.0f; // [m]

  // fprintf(stderr, "%s aabb w h l %g x %g x %g m\n",
  //     sx.assets.object[objectid].filename,
  //     w, h, l);

  // init inertia tensor
  //                | y^2 + z^2  -xy         -xz        |
  // I = int rho dV |-xy          x^2 + z^2  -xz        |
  //                |-xz         -yz          y^2 + x^2 |
  const float mass = rho * w * h * l;
  sx.world.entity[eid].body.m = mass;
  sx.world.entity[eid].body.invI[0] = 3.0f/8.0f / (rho * w * (h * l*l*l + l * h*h*h));
  sx.world.entity[eid].body.invI[4] = 3.0f/8.0f / (rho * h * (w * l*l*l + l * w*w*w));
  sx.world.entity[eid].body.invI[8] = 3.0f/8.0f / (rho * l * (w * h*h*h + h * w*w*w));

  sx_move_init(sx.world.entity + eid);
  sx_plot_init(sx.world.entity + eid);
  // fprintf(stderr, "[world] new entity %u\n", eid);
  return eid;
}

int
sx_world_init_terrain(
    const char *filename,
    uint32_t    elevation,
    uint32_t    cloud_height,
    uint32_t    terrain_scale)
{
  char fn[256];
  png_from_pcx(filename, fn, sizeof(fn));
  uint8_t *buf = 0;
  if(png_read(fn, &sx.world.terrain_wd, &sx.world.terrain_ht, (void**)&buf, &sx.world.terrain_bpp))
  {
    fprintf(stderr, "[world] failed to open `%s'\n", filename);
    return 1;
  }
  // TODO: this is exactly the one we might want to allow 16 bits!
  assert(sx.world.terrain_bpp == 8);
  sx.world.terrain = malloc(sizeof(uint8_t *)*sx.world.terrain_wd*sx.world.terrain_ht);
  for(int k=0;k<sx.world.terrain_wd*sx.world.terrain_ht;k++)
    sx.world.terrain[k] = buf[4*k];
  free(buf);
  sx.world.cloud_height = cloud_height;
  sx.world.terrain_low  = ft2m(elevation);
  sx.world.terrain_high = ft2m(
      768.0f * powf(2.0f, terrain_scale - 21.0));
  return 0;
}

void sx_world_think(const uint32_t dt)
{
  for(int i=0;i<sx.world.num_entities;i++)
    if(sx.world.entity[i].move.think)
      sx.world.entity[i].move.think(sx.world.entity+i);
}
