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

  // world space ray direction
  float angle_x = -u_hfov * (tex_coord.x - 0.5);
  float angle_y =  u_vfov * (tex_coord.y - 0.5);
  // equi-angular:
  vec3 camray = vec3(sin(angle_x)*cos(angle_y), sin(angle_y), cos(angle_x)*cos(angle_y));
  vec3 w = normalize(quat_transform(camray, u_orient));

  // draw artificial horizon, try to anti alias it a little:
  float wd = min(400.0, u_res.y*2.0);
  float aad = clamp(min(
      min(abs(w.y) * wd,
          abs(abs(w.y)-1.0) * wd),
      min(abs(abs(w.y)-M_PI/8.0)  * wd,
          abs(abs(w.y)-M_PI/16.0) * wd)), 0.0f, 1.0f);
  if(aad < 1.0f)
  {
    // project quaternion to get yaw:
    vec3 front = normalize(quat_transform(vec3(0.0f, 0.0f, 1.0f), u_orient));
    front = normalize(vec3(front.x, 0.0f, front.z));
    vec3 projray = normalize(vec3(w.x, 0.0f, w.z));
    float d = dot(projray, front);
    if((abs(w.y) < 0.01 && d > M_PI/4.0) || d > 0.95)
      frag_color = mix(u_col, frag_color, aad);
  }
}
