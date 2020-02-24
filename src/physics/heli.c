#include "physics/heli.h"
#include "sx.h"
#include "matrix3.h"
#include "triggers.h"

#if 0
1ft= 0.3048000m
ft =m * 3.2808
bounding box of comanche main rotor 39.2232 1.36477 37.7626
bounding box of comanche body 9.29609 10.3559 44.1551

// wikipedia says about the data:

General characteristics

Crew: two
Length: 46.85 ft (14.28 m)
Rotor diameter: 39.04 ft (11.90 m)             // matches straight out of 3do measure, yay!
Height: 11.06 ft (3.37 m)
Disc area: 1,197 ft² (111 m²)
Empty weight: 9,300 lb (4,218 kg)
Useful load: 5,062 lb (2,296 kg)
Loaded weight: 12,349 lb (5,601 kg)
Max. takeoff weight: 17,408 lb (7,896 kg)
Fuselage length: 43.31 ft (13.20 m)            // close enough to our 44.1 above.
Rotor systems: 5-blade main rotor, 8-blade Fantail tail rotor
Powerplant: 2 × LHTEC T800-LHT-801 turboshaft, 1,563 hp (1,165 kW) each

Performance
Maximum speed: 175 knots (201 mph, 324 km/h)
Cruise speed: 165 knots (190 mph, 306 km/h)
Range: 262 nmi (302 mi, 485 km)  on internal fuel
Combat radius: 150 nmi (173 mi, 278 km)  on internal fuel
Ferry range: 1,200 nmi (1,380 mi, 2,220 km)
Endurance: 2.5 hr
Service ceiling: 14,980 ft (4,566 m)
Rate of climb: 895 ft/min (4.55 m/s)
Disc loading: 10.32 lb/ft² (50.4 kg/m2)

Armament
1× Turreted Gun System with a 20 mm XM301 three-barrel rotary cannon (capacity: 500 rounds)
Internal bays: 6× AGM-114 Hellfire air-to-ground missiles, or 12× AIM-92 Stinger air-to-air missiles, or 24× 2.75 in (70 mm) Hydra 70 air-to-ground rockets
Optional stub wings: 8× Hellfires, 16× Stingers, or 56× Hydra 70 rockets
#endif

static inline void
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

void sx_heli_init(sx_heli_t *hl, sx_entity_t *ent)
{
  memset(hl, 0, sizeof(*hl));
  hl->ctl.collective = 0.25;
  hl->entity = ent;

  hl->ctl.gear = 1; // start with expanded gear

  // our coordinate system is
  //  x : left, y : up, z : front
  // all lengths are in meters

  // Empty weight: 9,300 lb (4,218 kg)
  // Useful load: 5,062 lb (2,296 kg)
  // Loaded weight: 12,349 lb (5,601 kg)
  // Max. takeoff weight: 17,408 lb (7,896 kg)
  // 900kg fuel ?
  // one main rotor blade is supposed to be anything from 30 to 100 kg.
  // let's assume we have 5 heavy blades, so 800 kg main rotor.
  // the armoured tail would be like 418 kg

  hl->useful_weight = 5601; // kg
  hl->service_height = 4566.0f; // m

  // sizes in meters:
  // bounding box of comanche main rotor 11.9552 0.415981 11.51
  // bounding box of comanche body 2.83345 3.15649 13.4585

  // let's assume we have a heavy engine and fuel tank close to the rotor:
  float c0[] = {0, -.5, .5}, ax0[] = {1.0, 1.0, 3};
  float M0 = 2100; // kg + fuel tank?

  // main rotor:
  hl->main_rotor_radius = 11.9f/2.0f;
  float c1[] = {0, 0, 0}, ax1[] = {hl->main_rotor_radius, 0.01, hl->main_rotor_radius};
  float M1 = 800; // kg

  // tail rotor:
  float c2[] = {0, -2.3, -6.8}, ax2[] = {0.8, 1.0, 0.3};
  float M2 = 418; // kg

  // cargo: (max load is 2296 kg)
  // TODO: distinguish between in bay and exterior!
  hl->cargo_left_weight  = 50;
  hl->cargo_right_weight = 50;
  float c3[] = {-2, -1.8, 0}, ax3[] = {0.5, 0.5, 2}; // left
  float M3 = hl->cargo_left_weight; // kg
  float c4[] = { 2, -1.8, 0}, ax4[] = {0.5, 0.5, 2}; // right
  float M4 = hl->cargo_right_weight; // kg

  // this puts our total center of mass to:
  for(int k=0;k<3;k++)
    hl->cmm[k] = (c0[k]*M0 + c1[k]*M1 + c2[k]*M2 + c3[k]*M3 + c4[k]*M4)/(M0+M1+M2+M3+M4);

  // okay, but we want it to be exactly under the main rotor, so let's try again:
  c0[2] -= hl->cmm[2]*(M0+M1+M2+M3+M4)/M0;
  // also, we'd like the tail rotor to be at the center in y direction, so:
  c0[1] -= (hl->cmm[1]-c2[1])*(M0+M1+M2+M3+M4)/M0;
  for(int k=0;k<3;k++)
    hl->cmm[k] = (c0[k]*M0 + c1[k]*M1 + c2[k]*M2 + c3[k]*M3 + c4[k]*M4)/(M0+M1+M2+M3+M4);
  fprintf(stderr, "[heli trim] center of mass: %g %g %g\n", hl->cmm[0], hl->cmm[1], hl->cmm[2]);

  // and the centers of the others wrt to the total center of mass:
  for(int k=0;k<3;k++) c0[k] -= hl->cmm[k];
  for(int k=0;k<3;k++) hl->mnr[k] = c1[k] -= hl->cmm[k];
  for(int k=0;k<3;k++) hl->tlr[k] = c2[k] -= hl->cmm[k];
  for(int k=0;k<3;k++) c3[k] -= hl->cmm[k];
  for(int k=0;k<3;k++) c4[k] -= hl->cmm[k];
  hl->main_rotor_weight = M1;
  hl->tail_rotor_weight = M2;
  fprintf(stderr, "[heli trim] c fuselage %g %g %g\n", c0[0], c0[1], c0[2]);
  fprintf(stderr, "[heli trim] c main rotor %g %g %g\n", c1[0], c1[1], c1[2]);
  fprintf(stderr, "[heli trim] c tail rotor %g %g %g\n", c2[0], c2[1], c2[2]);

  // each ellipsoid comes with their own inertia tensors:
  float I0[9], I1[9], I2[9], I3[9], I4[9];
  inertia_ellipsoid(ax0, c0, M0, I0);
  inertia_ellipsoid(ax1, c1, M1, I1);
  inertia_ellipsoid(ax2, c2, M2, I2);
  inertia_ellipsoid(ax3, c3, M3, I3);
  inertia_ellipsoid(ax4, c4, M4, I4);

  ent->body.m = M0+M1+M2+M3+M4;
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
  mat3_inv(I, ent->body.invI);

  // our mental model for air drag is a stupid axis aligned box in object space:
  {
  const float *aabb = sx.assets.object[ent->objectid].geo_aabb[1]; // just the bl1 rotor
  float w = aabb[3]-aabb[0], h = aabb[5]-aabb[2], l = aabb[4]-aabb[1];
  fprintf(stderr, "bounding box of comanche main rotor %g %g %g\n", w, h, l);
  }
  const float *aabb = sx.assets.object[ent->objectid].geo_aabb[0]; // just the body
  float w = aabb[3]-aabb[0], h = aabb[5]-aabb[2], l = aabb[4]-aabb[1];
  fprintf(stderr, "bounding box of comanche body %g %g %g\n", w, h, l);

  hl->dim[0] = w; // x width
  hl->dim[1] = h; // y height (up axis)
  hl->dim[2] = l; // z length of object

  // drag coefficients
  hl->cd[0] = 0.80;  // heavy drag from the sides (vstab)
  hl->cd[1] = 0.10;  // heavy but not quite as much vertically
  hl->cd[2] = 0.04;  // streamlined forward

  // our coordinate system is x left, y up, z front (right handed)
  // surfaces seem to work better (torque?) if we transform them:
  // x - drag
  // y - root -> tip
  // z - lift
  // regardless of handedness

  // add some stabilisers for nicer flight experience:
  hl->surf[s_heli_surf_vstab] = (sx_aerofoil_t){
    .orient = { // columns are x y z vectors
      0, 0, -1,
      0, 1,  0,
      1, 0,  0,
    },
    .root = {0.0f, 1.0f, hl->tlr[2]-0.4f},
    .chord_length = 0.5f,
    .span = 1.0f,
    .c_drag = 0.2f,
    .c_lift = 2.2f,
  };
  hl->surf[s_heli_surf_hstab_l] = (sx_aerofoil_t){
    .orient = {
      0, 1, 0,
      0, 0, 1,
      1, 0, 0,
    },
    .root = {0.0f, 2.1f, hl->tlr[2]-1.1f},
    .chord_length = 0.4f,
    .span = 1.4f,
    .c_drag = 0.2f,
    .c_lift = 0.8f,
  };
  hl->surf[s_heli_surf_hstab_r] = (sx_aerofoil_t){
    .orient = {
      0, -1, 0,
      0,  0, 1,
      1,  0, 0,
    },
    .root = {0.0f, 2.1f, hl->tlr[2]-1.1f},
    .chord_length = 0.4f,
    .span = 1.4f,
    .c_drag = 0.2f,
    .c_lift = 0.8f,
  };

  for(int i=0;i<s_heli_surf_cnt;i++)
    sx_aerofoil_init_orient(hl->surf+i);
}

float sx_heli_groundlevel(const sx_rigid_body_t *b)
{
  return sx_world_get_height(b->c);
}

float sx_heli_alt_above_ground(const sx_heli_t *h)
{
  // interested in height of gear above ground:
  return h->entity->body.c[1] - sx_heli_groundlevel(&h->entity->body) - 2.09178;
}

// TODO: will need replacement
void sx_heli_damage(sx_entity_t *ent, float x[3], float p[3])
{
  sx_heli_t *h = ent->move_data;
  float new_hitpoints = h->entity->hitpoints;
  float impulse = sqrtf(dot(h->entity->body.pv, h->entity->body.pv));
  if(impulse > 15000.0) // impulse in meters/second * kg
  {
    // TODO: more selective damage
    if(h->ctl.gear == 1)
    {
      if(impulse > 100000.0)
      {
        sx_sound_play(sx.assets.sound + sx.mission.snd_hit, -1);
        new_hitpoints -= 20; // TODO: damage gear
      }
    }
    else
    {
      sx_sound_play(sx.assets.sound + sx.mission.snd_hit, -1);
      new_hitpoints -= 5;
      if(impulse > 50000.0)
        new_hitpoints -= 100;
    }
  }
  // TODO: lose trigger
  // died
  if(new_hitpoints <= 0.0 && h->entity->hitpoints > 0.0)
  {
    sx_sound_play(sx.assets.sound + sx.mission.snd_explode, -1);
    sx.cam.mode = s_cam_rotate;
    sx.mission.gamestate = C3_GAMESTATE_LOSE;
    sx_sound_loop(sx.assets.sound+sx.mission.snd_fire, 5, 1000);
    char filename[32];
    c3_triggers_parse_music(filename, sx.mission.music, C3_GAMESTATE_LOSE, 'f');
    sx_music_play(sx_assets_filename_to_music(&sx.assets, filename), -1);
  }
  h->entity->hitpoints = new_hitpoints;
}

void sx_heli_update_forces(sx_entity_t *ent, sx_rigid_body_t *b)
{
  sx_heli_t *h = ent->move_data;
  sx_actuator_t *a;

  // gravity:
  a = h->act + s_act_gravity;
  // force = (0,-1,0) * 9.81 * b->m and transform by quaternion
  a->f[0] = 0.0f;
  a->f[1] = -9.81f * b->m;
  a->f[2] = 0.0f;
  a->r[0] = a->r[1] = a->r[2] = 0.0f; // applied equally everywhere

  // cyclic:
  a = h->act + s_act_cyclic;
  a->r[0] = h->mnr[0];
  a->r[1] = h->mnr[1];
  a->r[2] = h->mnr[2];
  a->f[0] = -.3f*h->ctl.cyclic[0]; // du: to the sides
  a->f[2] =  .3f*h->ctl.cyclic[1]; // dv: front or back
  a->f[1] = 1.0f;
  normalise(a->f);
  const float mr = 400.0f*h->main_rotor_weight / (b->m - h->main_rotor_weight);
  // main rotor runs at fixed rpm, so we get some constant anti-torque:
  // constant expressing mass ratio rotor/fuselage, lift vs torque etc
  float main_rotor_torque[3] = {0, mr*a->f[1], 0};
  const float main_rotor_torque1_os = main_rotor_torque[1]; //main rotor torque around y in object space
  quat_transform(&b->q, a->r); // to world space
  quat_transform(&b->q, a->f); // to world space
  quat_transform(&b->q, main_rotor_torque);
  // main thrust gets weaker for great heights
  // in ground effect, fitted to a graph on dynamicflight.com says thrust improves by
  const float hp = (b->c[1]-sx_heli_groundlevel(b)) / h->main_rotor_radius; // height above ground in units of rotor diameter
  // const float inground = hp < 3.0f ? 35.f*CLAMP(.5-(hp-2.0f)/(2.0f*sqrtf(1.0f+(hp-2.0f)*(hp-2.0f))), 0.0f, 1.0f) : 0.0f;
  const float inground = hp < 3.0f ? 18.0f*CLAMP(3.0f-hp, 0, 3.0f) : 0.0f;
  // height in meters where our rotor becomes ineffective (service height)
  // TODO: some soft efficiency with height?
  const float cutoff = b->c[1] > h->service_height ? 0.0f : 1.0f; // hard cutoff
  const float percent = (100.0 + inground) * h->ctl.collective; // where 70 means float out of ground, 60 float in ground ~25m
  for(int k=0;k<3;k++) a->f[k] *= cutoff * 9.81f*b->m * percent / 70.0f;
  
  // tail rotor:
  // x left, y up, z front of helicopter
  a = h->act + s_act_tail;
  a->f[0] = a->f[1] = a->f[2] = 0.0f;
  a->r[0] = h->tlr[0];
  a->r[1] = h->tlr[1];
  a->r[2] = h->tlr[2];
  a->f[0] = 0.30f * b->m * h->ctl.tail; // tail thrust
  // want to counteract main rotor torque, i.e. r x f == - main rotor torque
  a->f[0] -= main_rotor_torque1_os/a->r[2];
  quat_transform(&b->q, a->r); // to world space
  quat_transform(&b->q, a->f); // to world space

#if 1
  {
  // drag:
  // force = 0.5 * air density * |u|^2 * A(u) * cd(u)
  // where u is the relative air/object speed, cd the drag coefficient and A the area
  a = h->act + s_act_drag;
  a->r[0] = 0;
  a->r[1] = 0;
  a->r[2] = 0;
  // quat_transform(&b->q, a->r); // to world space
  float wind[3] = {0.0f, 0.0f, 0.0f};
  // mass density kg/m^3 of air at 20C
  const float rho_air = 1.204;
  for(int k=0;k<3;k++) wind[k] -= b->v[k];
  float vel2 = dot(wind, wind);
  if(vel2 > 0.0f)
  {
    const float scale = 2.0f; // arbitrary effect strength scale parameter
    quat_transform_inv(&b->q, wind); // to object space
    for(int k=0;k<3;k++)
      a->f[k] = scale * h->cd[k] * wind[k] * .5 * rho_air * vel2;
    quat_transform(&b->q, a->f); // to world space
  }
  else a->f[0] = a->f[1] = a->f[2] = 0.0f;
  }
#endif
  // dampen velocity
  // m/s -> nautical miles / h
  // XXX TODO: this needs to be limited by air flow against rotor blades i think
  if(ms2knots(sqrtf(dot(b->v, b->v))) < 170)
    b->drag = 0.0f;
  else b->drag = 0.95f;
  // b->angular_drag = 0.05f;

  float wind[3] = {0.0f};
  // TODO: force depends on speed of air relative to pressure point (in the absence
  // of wind it's the speed of the pressure point, include rotation!)
  // b->w: angular speed rad/s in world space around the three axes
  // off: the offset of the pressure point in world space around center of mass
  // lv: linear velocity of the pressure point in world space
  // float lv[3];
  // cross(b->w, off, lv);
  // for(int k=0;k<3;k++) wind[k] -= lv[k];
  for(int k=0;k<3;k++) wind[k] -= b->v[k];
  for(int i=0;i<s_heli_surf_cnt;i++)
    sx_aerofoil_apply_forces(h->surf+i, b, wind);

  sx_rigid_body_apply_force(b, h->act+s_act_gravity);
  if(h->entity->hitpoints > 0.)
  {
    sx_rigid_body_apply_torque(b, main_rotor_torque);
    sx_rigid_body_apply_force(b, h->act+s_act_cyclic);
  }
  sx_rigid_body_apply_force(b, h->act+s_act_tail);
  sx_rigid_body_apply_force(b, h->act+s_act_drag);
}
