#include "sx.h"
#include "physics/rigidbody.h"
#include "physics/heli.h"
#include "pngio.h"
#include "comanche.h"

#include <stdint.h>

// TODO: what part of the loading is done when entering a mission?

// TODO: interface to add an object? triggered by .pos loading?

void sx_world_init()
{
}

void sx_world_cleanup()
{
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

  for(int k=0;k<3;k++)
    e->prev_x[k] = e->body.c[k];
  e->prev_q = e->body.q;

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

static int
sx_world_collide_terrain(
    sx_entity_t *e) // use only coarse aabb
    // we'll resolve this internally i think:
    // float n[3],           // contact normal TODO: what space
    // float x[3],           // contact point  TODO: what space?
    // float o[3])           // offset to resolve interpenetration TODO: what space? ws?
{
  if(!e->move_data) return 0;
  if(!e->move.update_forces) return 0;

  // check a few points of the aabb-ellipsoid against collision with the ground
  // TODO: can we have rotors, too? or do we delegate that to the individual objects?
  // checking rotors to ground is arguably not a major concern.

  // TODO: damage relevant points? define in interface?
  // TODO: could also have points for gear/docking etc

  // hard clamp and set external forces to 0?
  sx_rigid_body_t *body = &e->body;
  const float groundlevel = sx_world_get_height(body->c);
  // TODO: none of this considers rotation, bounding boxes, etc:
  const float top = /* center of mass offset?*/-3.0f-sx.assets.object[e->objectid].geo_aabb[0][1]; // just the body
  const float ht = groundlevel + top;

  if(body->c[1] >= ht) return 0;
  // fprintf(stderr, "bam! %g %g %g\n", body->c[1], groundlevel, ht);
  // sound_play_random(rt.sound);
  // heavily dampen non-up orientation:
  body->c[1] = ht;
  e->prev_x[1] = ht;

  // TODO: use normal of terrain
  quat_t q0 = body->q;
  quat_t q1;
  quat_init(&q1, q0.w, 0.0f, q0.x[1], 0.0f);
  quat_normalise(&q1);
  // quat_slerp(&q0, &q1, 1-powf(1-.02f, dt/0.02f), &body->q);
  body->q = q1;
  // remove angular momentum and reduce speed
  for(int k=0;k<3;k++)
    body->pv[k] = body->pw[k] = 0.0f;
  // body->pv[0] *= .01f;
  // body->pv[2] *= .013f;
  // body->pv[1] = MAX(0.1*body->pv[1], .0f); // this is in object space, but we messed with orientation, too
  return 1;
}

// TODO: collision detection and interaction between objects
void sx_world_move(const uint32_t dt_milli)
{
  sx_vid_clear_debug_line();
  const float dt = dt_milli / 1000.0f; // delta t in seconds

  // move entities based on the forces we collected during the last iteration
#ifdef _OPENMP
#pragma omp parallel for
#endif
  for(int i=0;i<sx.world.num_entities;i++)
  {
    sx_world_move_entity(sx.world.entity + i, dt);
    // collide everybody with the ground plane. that's easy because the ground doesn't move:
    // TODO:
    sx_world_collide_terrain(sx.world.entity + i);
    // TODO: insert into 2D? hashed grid using coarse bounding sphere
  }
  // TODO collision between entities
  // TODO: compute external forces from wind,

  // for all objects
  //   get damage points and check against bounding boxes of all other potentially overlapping
  //   objects (use hashed grid from above)

  // update camera after everything else, it may be homing/locked to objects:
  if(sx.cam.mode == s_cam_homing)
  {
    // x left; y up; z front;
    // const float off[3] = {0, 2.3, -18};
    const float off[3] = {3, 2.3, -18};
    // const float off[3] = {-5, -.5, 10};
    const float *pos = sx.world.entity[sx.world.player_entity].body.c;
    const quat_t *q = &sx.world.entity[sx.world.player_entity].body.q;
    float wso[3] = {off[0], off[1], off[2]};
    quat_transform(q, wso); // convert object to world space
    sx_camera_target(&sx.cam, pos, q, wso, 0.03f, 0.03f);
    sx_camera_lookat(&sx.cam, pos, 0.03f, 0.03f);
  }
  else if(sx.cam.mode == s_cam_inside_cockpit || sx.cam.mode == s_cam_inside_no_cockpit)
  {
    const float off[3] = {0, 1.40f, 2.9f}; // front pilot's head
    const float *pos = sx.world.entity[sx.world.player_entity].body.c;
    const quat_t *q = &sx.world.entity[sx.world.player_entity].body.q;
    quat_t tmp = *q; // look down a bit
    quat_t down; quat_init_angle(&down, 0.12f+sx.cam.angle_down, 1.0f, 0.0f, 0.0f);
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

  // LOD: discard stuff > 1k
  float d[3], md = 1000;
  for(int k=0;k<3;k++) d[k] = sx.world.entity[ei].body.c[k] - sx.cam.x[k];
  if(dot(d, d) > md*md) return;

  if(//sx.cam.mode == s_cam_inside_cockpit ||
     sx.cam.mode == s_cam_inside_no_cockpit)
    if(ei == sx.world.player_entity) return;
  // TODO: optimise down to one uint32_t comparison:
  // TODO: and support more draw calls than just 'coma'
  const char *dr = sx.assets.object[oi].draw;
  if(dr[0]=='e'&& dr[1]=='d'&& dr[2]=='i'&& dr[3]=='t') return; // only visible in edit mode
  if(dr[0]=='c'&& dr[1]=='o'&& dr[2]=='m'&& dr[3]=='a')
  {
    // delegate to comanche drawing routine (with alpha for rotor and so on)
    // this one also knows the center of mass vs. zero in model coordinates
    // TODO: other move for other helis
    return sx_coma_render(ei, sx.world.player_move);
  }

  // fprintf(stderr, "rendering entity oid: %u/%u\n", oi, sx.assets.num_objects);
  sx_entity_t *e = sx.world.entity + ei;

  const quat_t *mq = &e->body.q;
  const quat_t *omq = &e->prev_q;
  const float *mp = e->body.c;
  const float *omp = e->prev_x;

  sx_object_t *obj = sx.assets.object + oi;

  // TODO: determine LOD h m l?
  // const int lod = 0;
  // TODO: desert snow or green?
  int geo_beg = obj->geo_g + obj->geo_h;
  int geo_end = MAX(geo_beg+1, obj->geo_g + obj->geo_m);

  for(int g=geo_beg;g<geo_end;g++)
    sx_vid_render_geo(obj->geoid[g], omp, omq, mp, mq);
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
  // TODO: use 4x get_height and a cross product

}

void
sx_world_remove_entity(
    uint32_t eid)
{
  sx.world.num_entities--;
  sx_entity_t tmp = sx.world.entity[eid];
  sx.world.entity[eid] = sx.world.entity[sx.world.num_entities];
  sx.world.entity[sx.world.num_entities] = tmp;
}

uint32_t
sx_world_add_entity(
    uint32_t objectid,   // 3d model to link to
    float *p,            // position
    quat_t *q,           // orientation
    char id,             // mission relevant id
    uint8_t camp,        // good guy, bad guy, neutral
    uint32_t ground)     // should i ground this object or let it float?
{
  if(sx.world.num_entities >= sizeof(sx.world.entity)/sizeof(sx.world.entity[0]))
    return -1;
  int eid = sx.world.num_entities++;
  memset(sx.world.entity + eid, 0, sizeof(sx_entity_t));

  // geometry stuff:
  sx.world.entity[eid].objectid = objectid;
  sx.world.entity[eid].id = id;
  sx.world.entity[eid].camp = camp;
  // offset by bounding box of object (found in header).
  // note that the objects have a z-up system:
  if(ground)
  {
    const float *aabb = sx.assets.object[objectid].aabb;
    p[1] = sx_world_get_height(p) - aabb[2];
  }
  // works almost for most objects. the sub-geo have often times not been transformed to their final
  // locations. also for instance scud are still floating.
  // because, as it turns out: there's a stray triangle way underneath it,
  // fucking up the bounding box. possible that the deal is that most things should
  // be centered at height = 0, bridges differently, helicopters with 0 at rotor, ..?

  // physics stuff
  for(int k=0;k<3;k++) sx.world.entity[eid].prev_x[k] = p[k];
  sx.world.entity[eid].prev_q = *q;
  for(int k=0;k<3;k++) sx.world.entity[eid].body.c[k] = p[k];
  sx.world.entity[eid].body.q = *q;

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
