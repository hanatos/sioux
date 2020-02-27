#version 440
#define M_PI 3.1415926535897932384626433832795
uniform vec2 u_res;
uniform vec3 u_pos;
uniform vec4 u_orient;
uniform float u_hfov;
uniform float u_vfov;
layout(binding = 0) uniform sampler2D render;
out vec3 frag_color;
in vec2 tex_coord;
uniform vec3 u_col;
uniform vec3 u_flash_col;

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

void main()
{
  vec3 col = texture(render, tex_coord).rgb;
  frag_color = col;
}
