#version 450 core

uniform float u_alpha; // blend factor, how much weight does the old buffer get
uniform mat4 u_curr;
uniform mat4 u_prev;
uniform ivec2 u_res;

// in pixel_center_integer vec2 gl_FragCoord;

out vec4 frag_color;

// layout(binding = 0) uniform sampler3D old_flow;
// layout(binding = 1, rgba16f) uniform writeonly image3D new_flow;
layout(binding = 0) uniform sampler2D old_flow;
layout(binding = 1, rgba16f) uniform writeonly image2D new_flow;

// okay we really want vulkan so we can include files like these easily:
// TODO: import flight dynamics vorticity generation code

vec4 quat_inv(vec4 q)
{ 
  return vec4(-q.x, -q.y, -q.z, q.w); 
}

vec4 quat_mul(vec4 q1, vec4 q2)
{ 
  vec4 qr;
  qr.x = (q1.w * q2.x) + (q1.x * q2.w) + (q1.y * q2.z) - (q1.z * q2.y);
  qr.y = (q1.w * q2.y) - (q1.x * q2.z) + (q1.y * q2.w) + (q1.z * q2.x);
  qr.z = (q1.w * q2.z) + (q1.x * q2.y) - (q1.y * q2.x) + (q1.z * q2.w);
  qr.w = (q1.w * q2.w) - (q1.x * q2.x) - (q1.y * q2.y) - (q1.z * q2.z);
  return qr;
}

vec3 quat_transform(vec3 position, vec4 qr)
{ 
  vec4 qr_inv = quat_inv(qr);
  vec4 q_pos = vec4(position.x, position.y, position.z, 0);

  vec4 q_tmp = quat_mul(qr, q_pos);
  qr = quat_mul(q_tmp, qr_inv);

  return vec3(qr.x, qr.y, qr.z);
}

// init quaternion to rotate a0 -> a1
vec4 quat_init_rot(vec3 a0, vec3 a1)
{
  vec3 a = cross(a0, a1);
  float cos_alpha = dot(a0, a1);
  float alpha = acos(clamp(cos_alpha, -1, 1));
  float cos_a2 = cos(alpha/2.0);
  float sin_a2 = sqrt(1.0f-cos_a2*cos_a2);
  return vec4(a * sin_a2, cos_a2);
}

// propeller for lift in direction of axis
// TODO: parameter for spread size and thrust and rotation speed
vec3 potential_propeller(
    vec3 p,         // center of the propeller
    vec3 axis,      // axis to generate lift to
    float width,    // ~radius of the propeller
    float downwash, // strength of downwash
    float rotation, // speed of rotation
    vec3 x)
{
  x -= p;
  // rotate axis -> y axis using quaternions
  vec4 q = quat_init_rot(axis, vec3(0, 1, 0));
  x = quat_transform(x, q);
  // spiral profile: most velocity in ring around center
  float sp_sides = exp(-dot(x.xz,x.xz)/(width*width));
  float sp_down = -smoothstep(0.0f, -1.0, x.y);
  // thrust profile
  float dd = sp_sides;//clamp((width - length(x.xz))/width, 0, 1);
  // dd = sqrt(dd);
  vec3 phi = - dd*downwash*x.x * vec3(0, 0, 1)  // go down (y direction)
     - vec3(0, 1, 0) * rotation*sp_sides*sp_down;        // spiral around y direction (ccw)

  q = quat_inv(q);
  return quat_transform(phi, q);
}

vec3 potential_occluder(
    vec3 p,        // center of occluder
    vec3 axis,     // axis to rotate x-axis to
    vec3 radius,   // radii of ellipsoid occluder
    vec3 phi,      // potential so far
    vec3 x)        // point to evaluate
{
  x -= p;
  vec4 q = quat_init_rot(axis, vec3(1, 0, 0));
  x = quat_transform(x, q);
  x /= radius;
  vec3 n = normalize(x);
  float dist = length(x);
  float alpha = smoothstep(0.0f, 1.0f, dist);
  phi = quat_transform(phi, q);
  vec3 nphi = mix(-n * dot(n, phi), phi, alpha);
  q = quat_inv(q);
  return quat_transform(nphi, q);
}

vec3 potential_vortex(
    float R,      // radius of influence
    vec3 x_c,     // center point of vortex
    vec3 omega_c, // angular velocity of vortex
    vec3 x)
{
  // f(|x - x_c|/R) (R*R - ||x-x_c||^2)/2 omega_c
  // where f: smoothing kernel
  return smoothstep(1, 0, length(x-x_c)/R) * (R*R - dot(x-x_c,x-x_c))/2.0 * omega_c;
}

vec3 potential_vortex_ring(
    float R,       // radius of vortex around ring
    float r,       // radius of ring itself
    vec3 c,        // center of ring
    vec3 n,        // normal of ring
    vec3 x)        // point to evaluate
{
  // f(|x - x_c|/R) (R*R - ||x-x_c||^2)/2 omega_c
  // where
  // f: smoothing kernel
  // omega_c: angular velocity (tangent to curve)
  vec3 dist = x - c;
  dist -= n * dot(dist, n);
  float len = length(dist);
  // closest point on circular curve
  vec3 x_c = r/len * dist;
  vec3 omega_c = cross(n, normalize(x_c));
  return potential_vortex(R, c + x_c, omega_c, x);
}

vec3 potential_field(vec3 x)
{
  // rotation of downwash:
  vec3 phi = potential_vortex(
      30.0f,           // radius
      vec3(0,0,0),    // center
      vec3(0, -.7, 0), // angular velocity
      x);
  // vortex ring of main rotor
  // phi += potential_vortex_ring(
  //   15.0,        // radius of vortex around ring
  //   3.0,         // radius of ring itself
  //   vec3(0,3,0), // center of ring
  //   vec3(0,1,0), // normal of ring
  //   x);
  // main rotor
  // vec3 phi = potential_propeller(vec3(0, 0, 0), vec3(0, 1, 0), 10.0, 7.0, 4, x);
  // tail rotor
  // phi += potential_propeller(vec3(0, 0, -6.5), normalize(vec3(-1, 0.2, 0)), 0.5, 3.0, 3, x);
  // fuselage
  // phi = potential_occluder(vec3(0, 0, 0), vec3(1, 0, 0), vec3(3, 4, 8), phi, x);
  return phi;
#if 0
  // float dist = exp(-dot(x.xz,x.xz)/25.0);
  // return  - x.x * vec3(0, 0, 1) +  2*dist * x.y;

  float dist = exp(-dot(x.xz,x.xz)/25.0); // spiral profile: most velocity in ring around center
  return  - 2*x.x * vec3(0, 0, 1)  // go down (y direction)
  - vec3(0, 1, 0) * 10*dist;       // spiral around y direction (ccw)
#endif
#if 0
  // XXX TODO: develop occluder and propeller
  // strength modulates in x and z directions:
  float sz = 1;//0.1 + 0.4*(1+cos(x.z / 6.5 * 3.1));
  float sx = 1.0 * (0.1 + 0.4*(1-cos(x.x / 6.5 * 6.14)));
  vec3 phi = sz*sx - x.x * vec3(0, 0, 1);
  vec3 n = normalize(x);
  float dist = length(x.x);
  float alpha = smoothstep(0.0f, 3.0f, dist);
  return mix(-n * dot(n, phi), phi, alpha);
  // go down:
  vec3 vert = 0.3*vec3(0, -0.1 - (x.y+4), 0);
  // radially move to the outside
  vec3 side = 0.005*vec3(x.x, 0.0, x.z);
  // interpolate alongside downwash: t=0 is top, t>0 goes towards ground
  float t = 3 - x.y;
  return sx*sz*vert + t*t * side;
#endif
}

void potential_deriv(
    vec3 x,
    out vec3 dpdx,
    out vec3 dpdy,
    out vec3 dpdz)
{
  vec3 c = potential_field(x);
  dpdx = (c - potential_field(x + vec3(1e-4, 0, 0)))/1e-4;
  dpdy = (c - potential_field(x + vec3(0, 1e-4, 0)))/1e-4;
  dpdz = (c - potential_field(x + vec3(0, 0, 1e-4)))/1e-4;
}

// compute divergence free noise by using curl (grad x)
vec3 velocity_field(vec3 tc)
{
  vec3 x = (tc - .5)*30.0; // now in meters
  return x; // XXX
  vec3 dpdx, dpdy, dpdz;
  potential_deriv(x, dpdx, dpdy, dpdz);
  return vec3(
      dpdy.z - dpdz.y,
      dpdz.x - dpdx.z,
      dpdx.y - dpdy.x);
}

void main()
{
  ivec3 sz = ivec3(128);//imageSize(new_flow);
  // ivec3 id = ivec3(gl_GlobalInvocationID);
  // risky undertaking in 32 bits. but we should be fine for a 2k image.
  // nobody would be as crazy and run a 4k screen, right?
  int px = int(gl_FragCoord.x) * u_res.x + int(gl_FragCoord.y);
    ivec2 writeid = ivec2(px %2048, px / 2048);
    imageStore(new_flow, writeid, vec4(0, -10, 0, 0));

#if 0
  for(int i=0;i<2;i++)
  {
    int idx = 2*px + i;

    int wd = 128;
    int iz =  idx / (wd*wd);
    int iy = (idx - iz * wd*wd)/wd;
    int ix =  idx - iy * wd - iz * wd*wd;
    ivec3 id = ivec3(ix, iy, iz);
    // if(any(greaterThanEqual(id, sz))) continue;

    // TODO: compute old texture coordinate
    // position in model coordinates
    vec3 tc = id/vec3(sz);

    // vec3 prev_ws = (u_prev * vec4(tc, 1)).xyz;
    // vec3 curr_ws = (u_curr * vec4(tc, 1)).xyz;

    // vec3 old_v = textureLod(old_flow, old_tc, 0).xyz;
    vec3 vel = velocity_field(tc);
    vec3 v;// = mix(vel, old_v, u_alpha);
    // v = 5*(id-64.0)/128.0;//vel;//vec3(0, -10, 0);
    v = vec3(0, id.z, 0)*0.1;
    v = vec3(0, -1, 0);
    // if(id.z < 64) v = vec3(0,-1, 0);
    // if(tc.z > 0.00) return;//v = vec3(0);
    // v = 5*(tc-.5);//vel;//vec3(0, -10, 0);
    // TODO: maybe blending is really integration of velocity/impulse?
    // imageStore(new_flow, id, vec4(v, 0));
    // DEBUG use 2d texture
    ivec2 writeid = ivec2(idx - 2048*(idx/2048), idx / 2048);
    imageStore(new_flow, writeid, vec4(v, 0));
  }
#endif
  discard;
}
