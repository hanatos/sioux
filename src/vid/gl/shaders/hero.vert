#version 440

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 uvs;  // super wacky integer in 3rd channel..

uniform float u_time;
uniform mat4 u_mvp;
uniform mat4 u_mv;
uniform vec2 u_res;
uniform int u_frame;

out vec3 shading_normal;
out vec3 position_cs;
out vec4 old_pos4;
out vec3 tex_uv;

float hash1( float n )
{
  return fract( n*17.0*fract( n*0.3183099 ) );
}

void main()
{
  vec3 v, ov;

  // transform model to view:
  // flip axes to reflect: x right, y back, z up on the input model
  // position; // input model coordinates
  gl_Position = u_mvp * vec4(position, 1);
  old_pos4 = u_mvp * vec4(position, 1); // XXX TODO: bring back old positions for taa
  tex_uv = uvs;
  position_cs    = vec3(u_mv * vec4(position, 1));
  shading_normal = vec3(u_mv * vec4(normal, 0));
#if 0
  p.y = -p.y;
  p = p.xzy;
  p.x = -p.x; // want x to mean left so we are right handed for phys sim!
  // now p is x right, y up, z front (model coordinates)
  v  = quat_transform(p, u_q) + u_pos;
  ov = quat_transform(p, u_old_q) + u_old_pos;
  // now v is view space x left, y up, z front (and similar but rotated in world space)

  // ---------------------------------
  // record camera space position and normal
  position_cs = v;
  // need to flip because we're going to change handedness by doing -z below:
  vec3 nr = vec3(normal.x, -normal.z, normal.y);
  shading_normal = -quat_transform(nr, u_q);

  // ---------------------------------
  // view matrices for half cube map:
  // look into four quadrants for left right top bottom

  // flip fucking z axis for opengl
  v.z = - v.z;
  ov.z = - ov.z;
  v.x = - v.x;  // now x shall be right so we're right handed again
  v += vec3(0,0,0);
  // now coord system is:
  // x-right, y-up, z-out of screen (negative z into it, right handed)

  // (note that this is all uniform compute..)
  // need to rotate left or right just enough to cover half the total field of view:
  // looks correct when assuming +z forward, +x right:
  const float s = sin(u_hfov*0.25);
  const float c = cos(u_hfov*0.25);
  mat3 L = mat3( c, 0.0,  s,   0,  1,   0, -s,  0,  c);
  mat3 R = mat3( c, 0.0, -s,   0,  1,   0,  s,  0,  c); // right hand counter clockwise rotation of the vertices
  if     (u_cube_side == 0) { v = L*v; ov = L*ov; }
  else if(u_cube_side == 1) { v = R*v; ov = R*ov; }

  // now scale to accomodate field of view:
  const float n = .10, f = 10000.0;
  const float r = n*s/c, l = -r;
  // need some leeway to not cut off corners:
  const float t = 1.38 * n*tan(u_vfov*0.5), b = -t;
  mat4 P = mat4(2*n/(r-l),   0,             0,           0,
                0,           2*n/(t-b),     0,           0,
                (r+l)/(r-l), (t+b)/(t-b), -(f+n)/(f-n), -1,
                0,           0,           -2*f*n/(f-n),  0);

  const vec2 jitter = vec2(0);
  // const vec2 jitter = 50*vec2(hash1(u_time), hash1(u_time+1337)) * 2.0/u_res;
  // const vec2 old_jitter = vec2(hash1(u_frame-1), hash1(u_frame)) * 3.5/u_res;
  // const vec2 jitter = vec2(mod(u_frame, 1), mod(u_frame/2, 1)) * 2.0/u_res;
  // i think we want to pretend there was no jitter when computing
  // motion vectors. so we just use the same jitter on both:
  // const vec2 old_jitter = vec2(
  //     mod(u_frame-1, 1),
  //     mod((u_frame-1)/2, 1)
  //     ) * 0.5/u_res;
  gl_Position = P*vec4(v, 1) + vec4(jitter, 0, 0);
  old_pos4 = P*vec4(ov, 1) + vec4(jitter, 0, 0);

  tex_uv = uvs;
#endif
}
