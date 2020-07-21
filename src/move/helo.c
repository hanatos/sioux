#include "sx.h"
#include "world.h"
#include "physics/rigidbody.h"
#include "physics/aerofoil.h"
#include "physics/move.h"
#include "physics/obb_obb.h"
#include "move/common.h"
#include "gameplay.h"

// helicopter move routines

typedef enum sx_helo_damage_t
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
sx_helo_damage_t;

typedef enum sx_helo_surf_t
{
  s_helo_surf_vstab = 0,
  s_helo_surf_hstab_l,
  s_helo_surf_hstab_r,
  s_helo_surf_cnt,
}
sx_helo_surf_t;

typedef struct sx_move_helo_t
{
  sx_aerofoil_t surf[s_helo_surf_cnt];

  float dim[3]; // dimensions of box used as flight model
  float cd[3];  // drag coefficients for main axes

  float mnr[3]; // main rotor center, with center of mass as zero
  float tlr[3]; // tail rotor center, with center of mass as zero

  sx_helo_damage_t damage; // individual damage flags

  // all these affect the inertia tensor. consider updating in-flight!
  float main_rotor_radius;
  float main_rotor_weight;
  float tail_rotor_weight;

  float cargo_left_weight;
  float cargo_right_weight;

  float useful_weight;  // hover with 70% torque at this weight
  float service_height; // max altitude

  // damage system, controlling flight dynamics:
  float dmg_main_rotor;
  float dmg_tail_rotor;
  float dmg_fuselage;
  float dmg_gear;
  float dmg_bay;
  float dmg_wing_tail;
  float dmg_wing_left;
  float dmg_wing_right;
}
sx_move_helo_t;

static float
sx_helo_alt_above_ground(const sx_entity_t *e)
{
  // interested in height of gear above ground:
  return e->body.c[1] - sx_world_get_height(e->body.c) - 2.09178;
}

void
sx_move_helo_update_forces(sx_entity_t *e, sx_rigid_body_t *b)
{
  sx_move_helo_t *h = e->move_data;
  sx_actuator_t a;

  // gravity:
  // force = (0,-1,0) * 9.81 * b->m and transform by quaternion
  a.f[0] = 0.0f;
  a.f[1] = -9.81f * b->m;
  a.f[2] = 0.0f;
  a.r[0] = a.r[1] = a.r[2] = 0.0f; // applied equally everywhere
  sx_rigid_body_apply_force(b, &a);

  // cyclic:
  a.r[0] = h->mnr[0];
  a.r[1] = h->mnr[1];
  a.r[2] = h->mnr[2];
  a.f[0] = -.4f*e->ctl.cyclic[0]; // du: to the sides
  a.f[2] =  .4f*e->ctl.cyclic[1]; // dv: front or back
  a.f[1] = 1.0f;
  normalise(a.f);
  const float mr = 400.0f*h->main_rotor_weight / (b->m - h->main_rotor_weight);
  // main rotor runs at fixed rpm, so we get some constant anti-torque:
  // constant expressing mass ratio rotor/fuselage, lift vs torque etc
  float main_rotor_torque[3] = {0, mr*a.f[1], 0};
  const float main_rotor_torque1_os = main_rotor_torque[1]; // main rotor torque around y in object space
  quat_transform(&b->q, a.r); // to world space
  quat_transform(&b->q, a.f); // to world space
  quat_transform(&b->q, main_rotor_torque);

  // main thrust gets weaker for great heights
  // in ground effect, fitted to a graph on dynamicflight.com says thrust improves by
  const float hp = (b->c[1]-sx_helo_alt_above_ground(e)) / h->main_rotor_radius; // height above ground in units of rotor diameter
  // const float inground = hp < 3.0f ? 35.f*CLAMP(.5-(hp-2.0f)/(2.0f*sqrtf(1.0f+(hp-2.0f)*(hp-2.0f))), 0.0f, 1.0f) : 0.0f;
  const float inground = hp < 3.0f ? 18.0f*CLAMP(3.0f-hp, 0, 3.0f) : 0.0f;
  // height in meters where our rotor becomes ineffective (service height)
  // TODO: some soft efficiency with height?
  const float cutoff = b->c[1] > h->service_height ? 0.0f : 1.0f; // hard cutoff
  const float percent = (100.0 + inground) * e->ctl.collective; // where 70 means float out of ground, 60 float in ground ~25m
  for(int k=0;k<3;k++) a.f[k] *= cutoff * 9.81f*b->m * percent / 70.0f;
  if(e->hitpoints > 0.)
  {
    sx_rigid_body_apply_torque(b, main_rotor_torque);
    sx_rigid_body_apply_force(b, &a);
  }
  
  // tail rotor:
  // x left, y up, z front of helicopter
  a.f[0] = a.f[1] = a.f[2] = 0.0f;
  a.r[0] = h->tlr[0];
  a.r[1] = h->tlr[1];
  a.r[2] = h->tlr[2];
  a.f[0] = 0.60f * b->m * e->ctl.tail; // tail thrust
  // want to counteract main rotor torque, i.e. r x f == - main rotor torque
  a.f[0] -= main_rotor_torque1_os/a.r[2];
  quat_transform(&b->q, a.r); // to world space
  quat_transform(&b->q, a.f); // to world space
  sx_rigid_body_apply_force(b, &a);


  // drag:
  // force = 0.5 * air density * |u|^2 * A(u) * cd(u)
  // where u is the relative air/object speed, cd the drag coefficient and A the area
  a.r[0] = 0;
  a.r[1] = 0;
  a.r[2] = 0;
  // quat_transform(&b->q, a->r); // to world space
  float wind[3] = {0.0f, 0.0f, 0.0f};
  // mass density kg/m^3 of air at 20C
  const float rho_air = 1.204;
  for(int k=0;k<3;k++) wind[k] -= b->v[k];
  float vel2 = dot(wind, wind);
  if(vel2 > 0.0f)
  {
    float wind_os[3] = {wind[0], wind[1], wind[2]};
    float scale = 0.84f;          // arbitrary effect strength scale parameter
    scale += e->stat.gear * 0.8f; // gear out, more drag
    scale += e->stat.bay  * 0.7f; // bay open, more drag
    quat_transform_inv(&b->q, wind_os); // to object space
    for(int k=0;k<3;k++)
      a.f[k] = scale * h->cd[k] * wind_os[k] * .5 * rho_air * vel2;
    quat_transform(&b->q, a.f); // to world space
  }
  else a.f[0] = a.f[1] = a.f[2] = 0.0f;
  sx_rigid_body_apply_force(b, &a);

  // dampen velocity
  // m/s -> nautical miles / h
  // XXX TODO: this needs to be limited by air flow against rotor blades i think
  // if(ms2knots(sqrtf(dot(b->v, b->v))) < 170)
  //   b->drag = 0.0f;
  // else b->drag = 0.95f;
  // b->angular_drag = 0.05f;

  // TODO: force depends on speed of air relative to pressure point (in the absence
  // of wind it's the speed of the pressure point, include rotation!)
  // b->w: angular speed rad/s in world space around the three axes
  // off: the offset of the pressure point in world space around center of mass
  // lv: linear velocity of the pressure point in world space
  // float lv[3];
  // cross(b->w, off, lv);
  // for(int k=0;k<3;k++) wind[k] -= lv[k];
  for(int k=0;k<3;k++) wind[k] -= b->v[k];
  for(int i=0;i<s_helo_surf_cnt;i++)
    sx_aerofoil_apply_forces(h->surf+i, b, wind);
}

void
sx_move_helo_damage(
    sx_entity_t       *e,
    const sx_entity_t *collider,
    float              extra_damage)
{
  float new_hitpoints = e->hitpoints;
  if(!collider)
  { // terrain:
    const float groundlevel = sx_world_get_height(e->body.c);
    float top = /* center of mass offset?*/-4.0f-sx.assets.object[e->objectid].geo_aabb[0][1]; // just the body
    if(e->hitpoints <= 0)
      top = -2.5f-sx.assets.object[sx.mission.obj_dead_coma].geo_aabb[0][1]; // just the body
    const float ht = groundlevel + top;

    if(e->body.c[1] >= ht) return;

    e->body.c[1] = ht;
    e->prev_x[1] = ht;

    float n[3]; // normal of terrain
    sx_world_get_normal(e->body.c, n);
    float up[3] = {0, 1, 0}, rg[3];
    quat_transform(&e->body.q, up); // to world space
    float dt = dot(up, n);
    cross(up, n, rg);
    // detect degenerate case where |rg| <<
    float len = dot(rg, rg);
    if(len > 0.02)
    {
      quat_t q0 = e->body.q;
      quat_t q1;
      quat_init_angle(&q1, acosf(fminf(1.0, fmaxf(0.0, dt))), rg[0], rg[1], rg[2]);
      quat_normalise(&q1);
      quat_mul(&q1, &q0, &e->body.q);
    }
    // remove momentum
    for(int k=0;k<3;k++)
      e->body.pw[k] = 0.0f;
    e->body.pv[1] = 0.0f;
    e->body.pv[0] *= 0.5f;
    e->body.pv[2] *= 0.5f;

    // TODO: this sucks and needs replacement. do velocity and if rotor touched etc.
    // TODO: at the very least see whether we hit ground upside down
    const float mul = sx.world.entity + sx.world.player_entity == e ? 1.0 : 0.001; // NPC are stupid and fly into mountains all the time
    float impulse = mul * sqrtf(dot(e->body.pv, e->body.pv));
    // if(dt < 0.5 && impulse > 15000.0) // impulse in meters/second * kg
    {
      // TODO: more selective damage
      // if(dt > 0.5 && e->stat.gear == 1)// can't do that with current AI.
      if(e->stat.gear == 1)
      {
        if(impulse > 20000.0)
        {
          sx_sound_play(sx.assets.sound + sx.mission.snd_hit, -1);
          new_hitpoints -= 20; // TODO: damage gear
        }
      }
      else if(impulse > 1000.0)
      {
        sx_sound_play(sx.assets.sound + sx.mission.snd_scrape, -1);
        new_hitpoints -= 5;
        if(dt < 0.2 || impulse > 5000.0)
          new_hitpoints -= 100;
      }
    }
  }
  else // if (!collider)
    if(collider->parent != e) // rockets don't explode on owner
  {
    if(e->hitpoints <= 0) return; // no need to precisely intersect dead things
    // call into the move routines to obtain list of collidable obb with
    // sx_part_type_t description what it is
    sx_obb_t obb[3];
    sx_part_type_t pt[3];
    int np = e->plot.collide ? e->plot.collide(e, obb, pt) : 0;

    // TODO: call plot.collide for the other entity, too!
    // TODO: create appropriate damage to our system
    // TODO: apply forces
    sx_obb_t box1;
    sx_obb_get(&box1, collider, 0, -1); // get first element, without offset since we don't know any better
    for(int p=0;p<np;p++)
    {
      if(sx_collision_test_obb_obb(&box1, obb+p))
      {
        // fprintf(stderr, "collision %s -- %s hit part type %d\n",
        //     sx.assets.object[e->objectid].filename,
        //     sx.assets.object[collider->objectid].filename,
        //     pt[p]);
        new_hitpoints -= 40;
        const sx_entity_t *parent = collider;
        for(int i=0;i<10;i++)
          if(parent->parent) parent = parent->parent; else break;
        sx_sound_play(sx.assets.sound + sx.mission.snd_hit, -1);
        if(e->hitpoints > 0 && new_hitpoints <= 0)
        {
          sx_spawn_explosion(e);
          // for(int i=0;i<10;i++) sx_spawn_debris(e);
        }
        fprintf(stderr, "[coll] %s's part %d hit by %s's %s hitp %g\n",
             sx.assets.object[e->objectid].filename,
             pt[p],
             (!parent || parent->objectid == -1u) ? "(died)" :
             sx.assets.object[parent->objectid].filename,
             sx.assets.object[collider->objectid].filename,
             new_hitpoints);
      }
    }
  }
  if(e-sx.world.entity == sx.world.player_entity &&
     new_hitpoints <= 0.0 && e->hitpoints > 0.0)
  { // player died
    sx_sound_play(sx.assets.sound + sx.mission.snd_explode, -1);
    sx.mission.gamestate = C3_GAMESTATE_LOSE;
    sx_sound_loop(sx.assets.sound+sx.mission.snd_fire, 5, 1000);
    uint32_t eid = sx_spawn_fire(e);
    quat_t q, p;
    quat_init_angle(&q, M_PI/2.0, 0, 1, 0);
    p = sx.world.entity[eid].body.q;
    quat_mul(&p, &q, &sx.world.entity[eid].body.q);
  }
  e->hitpoints = new_hitpoints;
}

void
sx_move_helo_think(sx_entity_t *e)
{
  if(e->hitpoints <= 0.0f) return; // dead.

  // apply controls:
  if(e->ctl.trigger_gear > 0 && e->stat.gear < 1.0f)
    e->stat.gear += 1.0f/128.0f;
  if(e->ctl.trigger_gear < 0 && e->stat.gear > 0.0f)
    e->stat.gear -= 1.0f/128.0f;

  if(e->ctl.trigger_bay > 0 && e->stat.bay < 1.0f)
    e->stat.bay += 1.0f/128.0f;
  if(e->ctl.trigger_bay < 0 && e->stat.bay > 0.0f)
    e->stat.bay -= 1.0f/128.0f;

  if(e->ctl.trigger_fire)
  { // fire trigger pulled
    if(e->stat.bay >= 1.0f)
    { // bay is open!
      int player = sx.world.entity + sx.world.player_entity == e;
      float delay = player ? 250.0f : 1500.0f;
      if(sx.time - e->fire_time >= delay)
      { // also weapons are ready
        uint32_t eid = player ?
          sx_spawn_rocket(e):       // some leveling, we don't need auto aim,
          sx_spawn_hellfire(e);     // they do..
        uint32_t oid = e->objectid;
        e->fire_time = sx.time;
        if(oid != -1u)
        {
          uint32_t offi[3];
          int cnt = e->plot.weapons(offi, 3);
          if(cnt >= 3)
          {
            e->side = (e->side+1) & 1;
            int w = 1 + e->side; // alternate between rockets
            float off[] = {
              sx.assets.object[oid].geo_off[0][3*offi[w]+0] - e->cmm[0],
              sx.assets.object[oid].geo_off[0][3*offi[w]+1] - e->cmm[1],
              sx.assets.object[oid].geo_off[0][3*offi[w]+2] - e->cmm[2]};
            // fprintf(stderr, "off %d,%d -- %g %g %g\n", w, offi[w], off[0], off[1], off[2]);
            quat_transform(&e->body.q, off);
            for(int k=0;k<3;k++)
              sx.world.entity[eid].body.c[k] += off[k];
          }
        }
        e->ctl.trigger_fire = 0;
      } // else not ready, bad luck, try again next time
    }
    else
    { // open bay and wait
      e->ctl.trigger_bay = 1;
    }
  }

  e->stat.alt_above_ground = sx_helo_alt_above_ground(e);

  // set new controls: autopilot.
  float wp_dist = 0.0f;
  int cruise_mode = 0;
  if(e-sx.world.entity != sx.world.player_entity)
  {
    e->ctl.autopilot_alt = 1;
    e->ctl.autopilot_alt_target = 30.0f;
    e->ctl.autopilot_vel = 1;
    e->ctl.autopilot_rot = 1;
  }

  // aim for next waypoint
  float d[3];

  for(int t=0;t<2;t++)
  {
    int gid = e->curr_wpg - 'A';
    if(e-sx.world.entity == sx.world.player_entity) gid = 0;
    d[0] = sx.world.group[gid].waypoint[e->curr_wp][0] - e->body.c[0];
    d[2] = sx.world.group[gid].waypoint[e->curr_wp][1] - e->body.c[2];
    d[1] = 0.0f;
#if 0 // everybody go at player!
    d[0] = sx.world.entity[sx.world.player_entity].body.c[0] - e->body.c[0];
    d[2] = sx.world.entity[sx.world.player_entity].body.c[2] - e->body.c[2];
    d[1] = 0.0f;
#endif

    for(int k=0;k<3;k++)
      e->ctl.autopilot_vel_target[k] = d[k];
    wp_dist = sqrtf(dot(d, d));
    if(wp_dist < 50.0f)
    {
      e->prev_wp = e->curr_wp;
      e->curr_wp++;
      if(e->curr_wp >= sx.world.group[gid].num_waypoints) e->curr_wp = 0; // restart
      // fprintf(stderr, "\n[XXX] group %c switching to waypoint %d\n", e->id, e->curr_wp+1);
    }
    else break;
  }

  // TODO: factor these out into enemy behaviour header
  if(e-sx.world.entity != sx.world.player_entity)
  {
    { // collision avoidance:
      int gid = e->id - 'A';
      sx_entity_t *o = 0;
      for(int m=0;m<sx.world.group[gid].num_members;m++)
      {
        o = sx.world.group[gid].member[m];
        if(e != o) continue;
      }
      if(e->id == 'W') o = sx.world.entity + sx.world.player_entity;
      float dist[3] = {
        o->body.c[0] - e->body.c[0],
        o->body.c[1] - e->body.c[1],
        o->body.c[2] - e->body.c[2]};
      if(sqrt(dot(dist,dist)) < 200.0f && (dot(dist, o->body.v) < 0 || dot(dist, e->body.v) > 0))
      {
        // fprintf(stderr, "\n[XXX] group %c fleeing!\n", e->id);
        // aargh we need to flee!
        d[0] -= 100*dist[0];
        d[2] -= 100*dist[2];
      }
    }

    if(e->engaged != -1u)
    { // check whether still in range
      float dist[3] = {
        sx.world.entity[e->engaged].body.c[0] - e->body.c[0],
        sx.world.entity[e->engaged].body.c[1] - e->body.c[1],
        sx.world.entity[e->engaged].body.c[2] - e->body.c[2]};
      if(sqrtf(dot(dist, dist)) > 700.0f) e->engaged = -1u;
    }

    if(e->engaged == -1u)
    { // find entity to engage
      uint32_t coll_ent[10];
      int coll_cnt = 10;
      int enemy_camp = e->camp == 2 ? 1 : 2;
      float aabb[6] = {
        e->body.c[0] - 700.0f, e->body.c[1] - 700.0f, e->body.c[2] - 700.0f,
        e->body.c[0] + 700.0f, e->body.c[1] + 700.0f, e->body.c[2] + 700.0f};
      coll_cnt = sx_grid_query(&sx.world.grid, aabb, coll_ent, coll_cnt, 1<<enemy_camp);
      for(int k=0;k<coll_cnt;k++)
        if(sx.world.entity[coll_ent[k]].hitpoints > 0 && 
            coll_ent[k] != sx.world.player_entity)
        { e->engaged = coll_ent[k]; break; }
      for(int k=0;k<coll_cnt;k++) if(coll_ent[k] == sx.world.player_entity)
      { // prioritise player:
        sx_entity_t *E = sx.world.entity + coll_ent[k];
        if(E->hitpoints > 0)
        {
          // cannot see if too low 
          float dist[3] = {
            E->body.c[0] - e->body.c[0],
            E->body.c[1] - e->body.c[1],
            E->body.c[2] - e->body.c[2]};
          float d = sqrtf(dot(dist, dist));
          float factor = E->stat.alt_above_ground / 45.0f;
          factor *= 0.8 + E->stat.gear * 0.1 + E->stat.bay * 0.1;
          // fprintf(stderr, "\n*** distance %g < %g, factor %g alt %g\n\n", d, factor*700.0f, factor, E->stat.alt_above_ground);
          if(d < MAX(300.0f, factor * 700.0f))
          {
            e->engaged = coll_ent[k];
            break;
          }
        }
      }
    }

    if(e->engaged != -1u)
    { // attack!
      sx_entity_t *E = sx.world.entity + e->engaged;
      float dist[3] = {
        E->body.c[0] - e->body.c[0],
        E->body.c[1] - e->body.c[1],
        E->body.c[2] - e->body.c[2]};
      // within radar range? check if gear or bay out, detect later then!
      float factor = 1.0f;
      factor += E->stat.gear * 0.5 + E->stat.bay * 0.5;
      float len = sqrt(dot(dist,dist));
      if(len < 300.0f * factor)
      {
        // fprintf(stderr, "\n[XXX] group %c attacking group %c!\n", e->id, E->id);
        d[0] = dist[0];
        d[1] = 0.0f;
        d[2] = dist[2];

        // also shoot at them!
        float fwd[] = {0, 0, 1};
        quat_transform(&e->body.q, fwd);
        if(dot(fwd, dist)/len > 0.9)
          e->ctl.trigger_fire = 1;
      }
    }
  }

  wp_dist = sqrtf(dot(d, d));
  for(int k=0;k<3;k++) d[k] /= wp_dist;

  // keep same altitude above ground level
  if(e->ctl.autopilot_alt)
  {
    // if height is a primary variable, we have to solve a differential equation
    // that does not oscillate. we want the solution to be an exponential falloff,
    // i.e. h = h0 + dh * exp(- alpha * t)
    // the first derivative is vertical speed, and the second derivative is the
    // collective, offset by whatever is needed to hover at zero vertical speed.
    // h'' = dh * alpha^2 exp(- alpha * t) = (h - h0) * alpha^2
    // this turns out to be too little and too late
    // use h' to feed h''
    // h'  = - dh * alpha * exp( -alpha * t) = vws[1]
    // h'' = - alpha * vws[1]
    // TODO: upside down h0 vs h??
    const float *vws = e->body.v;
    float h0 = e->stat.alt_above_ground;
    float h  = e->ctl.autopilot_alt_target;
    float dh = h - h0;
    float hpp = dh * 0.02f; // from primal, h
    hpp -= 0.2 * vws[1]; // from vertical speed, h'
    // TODO: find c0? it will vary for different weight etc
    float c0 = 0.55; // = e->ctl.autopilot_alt_guess;
    e->ctl.collective = c0 + hpp;
    float alpha = 0.99;
    e->ctl.autopilot_alt_guess *= alpha;
    e->ctl.autopilot_alt_guess += (1.0f-alpha) * (e->ctl.collective + hpp);
  }
  if(e->ctl.autopilot_vel)
  {
    // while not at desired speed, hold cyclic at some more or less fixed
    // offset into the right direction. don't oversteer.
    // also consider nose orientation: if below horizon, anticipate we'll pick up speed!
    float fwd[3] = {0, 0, 1}, rgt[3] = {1, 0, 0};
    quat_transform(&e->body.q, fwd);
    quat_transform(&e->body.q, rgt);

    // panic mode:
    // if(abs(fwd[1]) > 0.3 || abs(rgt[1]) > 0.3 ||
    //     dot(e->body.v, e->body.v) > 400 || 
    //     dot(e->body.w, e->body.w) > 400)
    {
      cruise_mode = 0;
      e->ctl.autopilot_vel_guess[0] = 0;
      e->ctl.autopilot_vel_guess[1] = 0;
    }

    // cyclic is really like the third derivative or so..
    // unfortunately this will not work in the presence of wind.
    const float *v0 = d;//e->ctl.autopilot_vel_target;
    const float vel = CLAMP(0.5f*wp_dist, 0.0f, 30.0f);// cruise_mode ? 30.0f : 0.5f;
    float v0_os[3] = {v0[0]*vel, v0[1]*vel, v0[2]*vel};
    float v_os[3] = {e->body.v[0], e->body.v[1], e->body.v[2]};
    float w_os[3] = {e->body.w[0], e->body.w[1], e->body.w[2]};
    quat_transform_inv(&e->body.q, v_os);
    quat_transform_inv(&e->body.q, w_os);
    quat_transform_inv(&e->body.q, v0_os);
    if(cruise_mode)
    {
      // sideways is always trying to maintain a fraction of the yaw delta:
      e->ctl.cyclic[0] = -0.002f*w_os[2]; // dampen roll
      e->ctl.cyclic[0] -= 0.002f*w_os[1]; // dampen yaw
      float sin_roll = - .5f*v0_os[0] / vel;
      sin_roll *= fabsf(sin_roll);
      float val = 0.06f*CLAMP(rgt[1] - sin_roll, -.1, .1);
      e->ctl.cyclic[0] -= val;
      if(e-sx.world.entity == sx.world.player_entity)
      {
        fprintf(stderr, "rgt %g tgt %g\n", rgt[1], sin_roll);
        fprintf(stderr, "cyclic 0 = %g + %g + %g= %g\n",
            -0.002f*w_os[2],
            -0.002f*w_os[1],
            -val,
            e->ctl.cyclic[0]);
      }

      // now bias velocity to go almost straight forward:
      v0_os[2] += 60.0f;
      for(int k=0;k<3;k++) v0_os[k] *= 0.333f;

      e->ctl.cyclic[1]  = -0.001f*(v_os[2] - v0_os[2]);
      e->ctl.cyclic[1]  = CLAMP(e->ctl.cyclic[1], -.1f, .1f);
      e->ctl.cyclic[1] -= 0.28f*w_os[0];
      e->ctl.cyclic[1] += 0.28f*(fwd[1] - e->ctl.autopilot_vel_guess[0]);

      float alpha = 0.97;
      float dv = v_os[2] - v0_os[2];
      e->ctl.autopilot_vel_guess[0] *= alpha;
      e->ctl.autopilot_vel_guess[0] += (1.0f-alpha) * (fwd[1] - 0.001*dv);
      e->ctl.autopilot_vel_guess[0] = CLAMP( // never fly backwards
          e->ctl.autopilot_vel_guess[0], -0.2f, 0.0f);
    }
    else
    { // super conservative and careful, trying to hover in mid air
      // float v0_os[3] = {-5.0, 0, 0}; // XXX [0]:left [1]:up
      // now it is about v[0],v[2]
      e->ctl.cyclic[1]  = -0.001f*(v_os[2] - v0_os[2]);
      e->ctl.cyclic[1]  = CLAMP(e->ctl.cyclic[1], -.1f, .1f);
      e->ctl.cyclic[1] -= 0.28f*w_os[0];
      e->ctl.cyclic[1] += 0.28f*(fwd[1] - e->ctl.autopilot_vel_guess[0]);

      e->ctl.cyclic[0]  = 0.001f*(v_os[0] - v0_os[0]);
      e->ctl.cyclic[0]  = CLAMP(e->ctl.cyclic[0], -.1f, .1f);
      e->ctl.cyclic[0] -= 0.28f*w_os[2];
      e->ctl.cyclic[0] -= 0.28f*(rgt[1] - e->ctl.autopilot_vel_guess[1]);

      // need to find rgt[1] and fwd[1] in zero position for given target
      // velocity or in the presence of wind!
      // take exponential averages of nose coordinates and correlate to velocity!
      // allow airship to settle after cyclic input before checking correlation.
      float alpha = 0.99; // for exponential averaging, higher alpha is more conservative
      // let's guess where our nose would have to go for current speed.
      // if we're too slow, it's probably going to have to be a little lower.
      float dv = v_os[2] - v0_os[2];
      e->ctl.autopilot_vel_guess[0] *= alpha;
      e->ctl.autopilot_vel_guess[0] += (1.0f-alpha) * (fwd[1] - 0.001*dv);
      dv = v_os[0] - v0_os[0];
      e->ctl.autopilot_vel_guess[1] *= alpha;
      e->ctl.autopilot_vel_guess[1] += (1.0f-alpha) * (rgt[1] - 0.001*dv);
    }
  }
  if(e->ctl.autopilot_rot)
  { // auto tail rotor, explicitly. re-use velocity target point:
    float fwd[3] = {0, 0, 1}, to[3] = {d[0], d[1], d[2]};
    quat_transform(&e->body.q, fwd); // nose in world space

    fwd[1] = to[1] = 0.0f;
    normalise(fwd); normalise(to);
    float angle_we = atan2f(-fwd[0], fwd[2]);
    float angle_to = atan2f(-to [0], to [2]);
    e->ctl.tail = (angle_to - angle_we) * 1.0f;
    // dampen:
    e->ctl.tail += 3.f * e->body.w[1];
  }

  // sanitise all values
  e->ctl.collective = CLAMP(e->ctl.collective, 0.0f, 1.2f);
  e->ctl.cyclic[0]  = CLAMP(e->ctl.cyclic[0], -1.0f, 1.0f);
  e->ctl.cyclic[1]  = CLAMP(e->ctl.cyclic[1], -1.0f, 1.0f);
  e->ctl.tail       = CLAMP(e->ctl.tail,      -1.0f, 1.0f);
}

// TODO: put in some physics boilerplate thing
static void
inertia_ellipsoid(const float axis[3], const float c[3], const float M, float I[9])
{
  // compute inertia tensor around center of mass at 0:
  memset(I, 0, sizeof(float)*9);
  I[0] = 1.0f/5.0f * M * (axis[1]*axis[1] + axis[2]*axis[2]);
  I[4] = 1.0f/5.0f * M * (axis[0]*axis[0] + axis[2]*axis[2]);
  I[8] = 1.0f/5.0f * M * (axis[0]*axis[0] + axis[1]*axis[1]);

  // now shift to center of mass in new coordinate frame, such that our
  // previous 0 lands at c[]
  // [Dynamics of Rigid Bodies ]
  for(int j=0;j<3;j++) for(int i=0;i<3;i++)
    if(j != i) // off diagonal
      I[3*j+i] += M * c[j]*c[i];
    else // diagonal:
      I[3*j+i] += M * (c[(i+1)%3]*c[(i+1)%3] + c[(i+2)%3]*c[(i+2)%3]);
}


void
sx_move_helo_init(sx_entity_t *e)
{
  sx_move_helo_t *r = malloc(sizeof(*r));
  memset(r, 0, sizeof(*r));
  e->move_data = r;

  e->ctl.collective = 0.25;
  // start with expanded gear and closed bay
  e->ctl.trigger_gear = 1;
  e->stat.gear = 1;
  e->ctl.trigger_bay = -1;
  e->stat.bay = 0;

  // TODO: distinguish between type of helicopter here!
  // TODO: we have to support wing, helo, plyr
  // TODO: these could just overwrite the default think() function pointer

  // our coordinate system is
  //  x : left, y : up, z : front
  // all lengths are in meters

  // these are comanche data points:
  // Empty weight: 9,300 lb (4,218 kg)
  // Useful load: 5,062 lb (2,296 kg)
  // Loaded weight: 12,349 lb (5,601 kg)
  // Max. takeoff weight: 17,408 lb (7,896 kg)
  // 900kg fuel ?
  // one main rotor blade is supposed to be anything from 30 to 100 kg.
  // let's assume we have 5 heavy blades, so 800 kg main rotor.
  // the armoured tail would be like 418 kg

  r->useful_weight = 5601; // kg
  r->service_height = 4566.0f; // m

  // sizes in meters:
  // bounding box of comanche main rotor 11.9552 0.415981 11.51
  // bounding box of comanche body 2.83345 3.15649 13.4585

  // let's assume we have a heavy engine and fuel tank close to the rotor:
  float c0[] = {0, -.5, .5}, ax0[] = {1.0, 1.0, 3};
  float M0 = 2100; // kg + fuel tank?

  // main rotor:
  r->main_rotor_radius = 11.9f/2.0f;
  float c1[] = {0, 0, 0}, ax1[] = {r->main_rotor_radius, 0.01, r->main_rotor_radius};
  float M1 = 800; // kg

  // tail rotor:
  float c2[] = {0, -2.3, -6.8}, ax2[] = {0.8, 1.0, 0.3};
  float M2 = 418; // kg

  // cargo: (max load is 2296 kg)
  // TODO: distinguish between in bay and exterior!
  r->cargo_left_weight  = 50;
  r->cargo_right_weight = 50;
  float c3[] = {-2, -1.8, 0}, ax3[] = {0.5, 0.5, 2}; // left
  float M3 = r->cargo_left_weight; // kg
  float c4[] = { 2, -1.8, 0}, ax4[] = {0.5, 0.5, 2}; // right
  float M4 = r->cargo_right_weight; // kg

  // this puts our total center of mass to:
  for(int k=0;k<3;k++)
    e->cmm[k] = (c0[k]*M0 + c1[k]*M1 + c2[k]*M2 + c3[k]*M3 + c4[k]*M4)/(M0+M1+M2+M3+M4);

  // okay, but we want it to be exactly under the main rotor, so let's try again:
  c0[2] -= e->cmm[2]*(M0+M1+M2+M3+M4)/M0;
  // also, we'd like the tail rotor to be at the center in y direction, so:
  c0[1] -= (e->cmm[1]-c2[1])*(M0+M1+M2+M3+M4)/M0;
  for(int k=0;k<3;k++)
    e->cmm[k] = (c0[k]*M0 + c1[k]*M1 + c2[k]*M2 + c3[k]*M3 + c4[k]*M4)/(M0+M1+M2+M3+M4);
  // fprintf(stderr, "[heli trim] center of mass: %g %g %g\n", hl->cmm[0], hl->cmm[1], hl->cmm[2]);

  // and the centers of the others wrt to the total center of mass:
  for(int k=0;k<3;k++) c0[k] -= e->cmm[k];
  for(int k=0;k<3;k++) r->mnr[k] = c1[k] -= e->cmm[k];
  for(int k=0;k<3;k++) r->tlr[k] = c2[k] -= e->cmm[k];
  for(int k=0;k<3;k++) c3[k] -= e->cmm[k];
  for(int k=0;k<3;k++) c4[k] -= e->cmm[k];
  r->main_rotor_weight = M1;
  r->tail_rotor_weight = M2;
  // fprintf(stderr, "[heli trim] c fuselage %g %g %g\n", c0[0], c0[1], c0[2]);
  // fprintf(stderr, "[heli trim] c main rotor %g %g %g\n", c1[0], c1[1], c1[2]);
  // fprintf(stderr, "[heli trim] c tail rotor %g %g %g\n", c2[0], c2[1], c2[2]);

  // each ellipsoid comes with their own inertia tensors:
  float I0[9], I1[9], I2[9], I3[9], I4[9];
  inertia_ellipsoid(ax0, c0, M0, I0);
  inertia_ellipsoid(ax1, c1, M1, I1);
  inertia_ellipsoid(ax2, c2, M2, I2);
  inertia_ellipsoid(ax3, c3, M3, I3);
  inertia_ellipsoid(ax4, c4, M4, I4);

  e->body.m = M0+M1+M2+M3+M4;
  float I[9] = {0};
  // and the total inertia will be the sum of the above:
  for(int k=0;k<9;k++) I[k] = I0[k] + I1[k] + I2[k] + I3[k] + I4[k];
  // I[0] = I[4] = I[8] = ent->body.m;
  // fprintf(stderr, "I2 = \n");
  // fprintf(stderr, " %g %g %g \n", I2[0], I2[1], I2[2]);
  // fprintf(stderr, " %g %g %g \n", I2[3], I2[4], I2[5]);
  // fprintf(stderr, " %g %g %g \n", I2[6], I2[7], I2[8]);
  // fprintf(stderr, "I = \n");
  // fprintf(stderr, " %g %g %g \n", I[0], I[1], I[2]);
  // fprintf(stderr, " %g %g %g \n", I[3], I[4], I[5]);
  // fprintf(stderr, " %g %g %g \n", I[6], I[7], I[8]);
  mat3_inv(I, e->body.invI);

  // our mental model for air drag is a stupid axis aligned box in object space:
  const float *aabb = sx.assets.object[e->objectid].geo_aabb[0]; // just the body
  float w = aabb[3]-aabb[0], h = aabb[5]-aabb[2], l = aabb[4]-aabb[1];
  // fprintf(stderr, "bounding box of comanche body %g %g %g\n", w, h, l);

  r->dim[0] = w; // x width
  r->dim[1] = h; // y height (up axis)
  r->dim[2] = l; // z length of object

  // drag coefficients
  r->cd[0] = 0.80;  // heavy drag from the sides (vstab)
  r->cd[1] = 0.10;  // heavy but not quite as much vertically
  r->cd[2] = 0.04;  // streamlined forward

  // our coordinate system is x left, y up, z front (right handed)
  // surfaces seem to work better (torque?) if we transform them:
  // x - drag
  // y - root -> tip
  // z - lift
  // regardless of handedness

  // add some stabilisers for nicer flight experience:
  // TODO: use esoteric rotation of hstab for comanche
  r->surf[s_helo_surf_vstab] = (sx_aerofoil_t){
    .orient = { // columns are x y z vectors
      0, 0, -1,
      0, 1,  0,
      1, 0,  0,
    },
    .root = {0.0f, 1.0f, r->tlr[2]-0.4f},
    .chord_length = 0.6f,
    .span = 1.0f,
    .c_drag = 0.2f,
    .c_lift = 2.2f,
  };
  r->surf[s_helo_surf_hstab_l] = (sx_aerofoil_t){
    .orient = {
      0, 1, 0,
      0, 0, 1,
      1, 0, 0,
    },
    .root = {0.0f, 2.1f, r->tlr[2]-1.1f},
    .chord_length = 0.3f,
    .span = 1.3f,
    .c_drag = 0.2f,
    .c_lift = 0.8f,
  };
  r->surf[s_helo_surf_hstab_r] = (sx_aerofoil_t){
    .orient = {
      0, -1, 0,
      0,  0, 1,
      1,  0, 0,
    },
    .root = {0.0f, 2.1f, r->tlr[2]-1.1f},
    .chord_length = 0.3f,
    .span = 1.3f,
    .c_drag = 0.2f,
    .c_lift = 0.8f,
  };

  for(int i=0;i<s_helo_surf_cnt;i++)
    sx_aerofoil_init_orient(r->surf+i);
}

