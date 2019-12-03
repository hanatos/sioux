#pragma once
#include "rigidbody.h"

// generic aerofoil to be added to aircraft
//
// chord length : projected length of the cross section (root, mean aerodynamic (MAC), tip), joining leading edge and trailing edge of the aerofoil
// camber line  : line inside the cross section, can be longer than chord length if cross section is curved/asymmetrical
// span         : length root to tip
// tapered wing : has a specific chord length deformation along the span (can be computed given area and taper ratio and span)
// sweep        : angle deviating from orthogonal to station line (front to back through fuselage)
// dihedral     : angle wrt fuselage as seen from front or back (up/down for side wings)
// incidence    : change angle of attack
// twist        : rotate cross section along the span
typedef struct sx_aerofoil_t
{
  float orient[9];     // orientation wrt fuselage (parent). v_fuselage = orient * v_aerofoil
  float root[3];       // position where aerofoil is attached to the fuselage, in fuselage coordinates
  float chord_length;  // chord length
  float span;          // span length

  float c_drag;        // drag coefficient
  float c_side_drag;   // side drag (towards tip of wing)
  float c_lift;        // lift coefficient

  // TODO bring in more from below
}
sx_aerofoil_t;

// initialise orientation matrix (orthonormalise)
static inline void
sx_aerofoil_init_orient(
    sx_aerofoil_t *foil)  // aerofoil to normalise orientation matrix (going local aerofoil system to object space)
{
  float x_os[3];  // drag vector (x forward)
  float y_os[3];  // root-to-tip vector, side drag direction
  float z_os[3];  // up vector, lift expected this way
  x_os[0] = foil->orient[0];
  x_os[1] = foil->orient[3];
  x_os[2] = foil->orient[6];
  y_os[0] = foil->orient[1];
  y_os[1] = foil->orient[4];
  y_os[2] = foil->orient[7];
  z_os[0] = foil->orient[2];
  z_os[1] = foil->orient[5];
  z_os[2] = foil->orient[8];

  // orthonormalise, want lift to point exactly where the engineer intended:
  normalise(z_os);
  float dxz = dot(x_os, z_os);
  for(int k=0;k<3;k++) x_os[k] -= dxz * z_os[k];
  normalise(x_os);
  float dyx = dot(y_os, x_os);
  for(int k=0;k<3;k++) y_os[k] -= dyx * x_os[k];
  float dyz = dot(y_os, z_os);
  for(int k=0;k<3;k++) x_os[k] -= dyz * z_os[k];
  normalise(y_os);

  // assemble matrix
  foil->orient[0] = x_os[0];
  foil->orient[3] = x_os[1];
  foil->orient[6] = x_os[2];
  foil->orient[1] = y_os[0];
  foil->orient[4] = y_os[1];
  foil->orient[7] = y_os[2];
  foil->orient[2] = z_os[0];
  foil->orient[5] = z_os[1];
  foil->orient[8] = z_os[2];
}

// yasim version (stolen from Surface::calcForce)
// Calculate the aerodynamic force given a wind vector v (in the
// aircraft's "local" coordinates), an air density rho and the freestream
// mach number (for compressibility correction).  Returns a torque about
// the Y axis ("pitch"), too.
static inline void
sx_aerofoil_apply_forces(
    const sx_aerofoil_t *foil,
    sx_rigid_body_t     *body,    // rigid body to apply our forces into
    const float         *wind)    // incident wind vector in world space
{
  float wind_os[3] = {wind[0], wind[1], wind[2]};
  quat_transform_inv(&body->q, wind_os); // in object space (i.e. airplane)

  float vel = sqrtf(dot(wind_os, wind_os));
  // catch degenerate case:
  if(vel < 1e-6f) return;
  for(int k=0;k<3;k++) wind_os[k] /= vel;

  // the object coordinate system   : +x left +y up +z front
  // the aerofoil coordinate system : +x drag back +y drag sideways right +z lift up

  // TODO: make surface properties
  const float cx = foil->c_drag;      // drag coefficient
  const float cy = foil->c_side_drag; // y-drag (sideways)
  const float cz = foil->c_lift;      // lift coefficient
  const float cz0 = 0.0f;             // zero aoa lift coefficient, as fraction of cz (caused by camber)
  const float induced_drag = 0.0f;
  const float peak[2]  = {1.0f, 1.0f};  // Stall peak coefficients (fwd, back)
  const float stall[4] = {0.36, 0.36, 0.36, 0.36};   // Stall angles (fwd/back, pos/neg)
  const float width[4] = {0.01f, 0.01f, 0.01f, 0.01f}; // Stall widths

  // no drag no lift: nothing to do:
  if(cx == 0.0f && cy == 0.0f && cz == 0.0f) return;

  float wind_as[3]; // wind in aerofoil coordinates
  mat3_tmulv(foil->orient, wind_os, wind_as);

#if 1
  float stall_mul = 1.0f;
  {
  // XXX tan(a) = a ??
  const float alpha = fabsf(wind_as[2]/wind_as[0]);

  // Wacky use of indexing, see setStall*() methods.
  int fwdBak = wind_as[0] > 0; // set if this is "backward motion"
  int posNeg = wind_as[2] < 0; // set if the airflow is toward -z
  int i = (fwdBak<<1) | posNeg;

  if(!(alpha == alpha) || stall[i] == 0) // pathological input
    ;
  else if(alpha > stall[i] + width[i]) // beyond stall
    ;
  else if(alpha <= stall[i]) // before stall
    stall_mul = 0.5f*peak[fwdBak]/stall[i&2];
  else // transition region
  {
    float scale = 0.5f*peak[fwdBak]/stall[i&2];
    float frac = (alpha - stall[i]) / width[i];
    frac = frac*frac*(3-2*frac);
    stall_mul = scale*(1-frac) + frac;
  }
  }
#endif
  // stallMul *= 1 + _spoilerPos * (_spoilerLift - 1);
  float stalllift = (stall_mul - 1) * cz * wind_as[2];
  float flaplift = 0.0f;//flapLift(out[2]);
  float force_as[3];
  force_as[2] = cz * wind_as[2] + cz*cz0 + stalllift + flaplift;

  // Airfoil lift (pre-stall and zero-alpha) torques "up" (negative
  // torque) around the Y axis, while flap lift pushes down.  Both
  // forces are considered to act at one third chord from the
  // edge.
  float torque_as[3];
  torque_as[0] = 0;
  torque_as[1] = 0.1667f * foil->chord_length * (flaplift - (cz*cz0 + stalllift));
  torque_as[2] = 0;

  // TODO: this should probably scale with angle of attack, too:
  force_as[0] = cx * wind_as[0]; // drag
  force_as[1] = cy * wind_as[1]; // side drag

  // handle induced drag
  for(int k=0;k<3;k++)
    force_as[k] -= induced_drag * force_as[2] * wind_as[2] * wind_as[k];

  float force_os[3], torque_os[3]; // to object space
  // force_as[0] = 0.0; // XXX DEBUG no drag
  // force_as[1] = 0.0; // XXX DEBUG no y drag
  // torque_as[0] = torque_as[1] = torque_as[2] = 0.0f; // XXX DEBUG no torque
  mat3_mulv(foil->orient, force_as,  force_os);
  mat3_mulv(foil->orient, torque_as, torque_os);

  const float c0 = 1.0f;
  const float A = foil->chord_length * foil->span;
  const float rho = 1.204f; // air
  const float scale = 0.5f*rho*vel*vel*c0 * A;
  for(int k=0;k<3;k++) torque_os[k] *= scale;
  for(int k=0;k<3;k++) force_os [k] *= scale;
  sx_actuator_t act = (sx_actuator_t) {
    .f = {force_os[0], force_os[1], force_os[2]},
  };
  float r[3] = {0.0f, .5f * foil->span, 0.0f};
  mat3_mulv(foil->orient, r, act.r);
  for(int k=0;k<3;k++) act.r[k] += foil->root[k];
  // transform to world space for integration:
  quat_transform(&body->q, act.f);
  quat_transform(&body->q, act.r);
  quat_transform(&body->q, torque_os);
  sx_rigid_body_apply_force (body, &act);
  sx_rigid_body_apply_torque(body, torque_os);
}

