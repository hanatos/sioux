#version 450 core

layout (location = 0) in vec3 position;

uniform vec2 u_terrain_bounds;
uniform float u_time;
uniform vec2 u_res;
uniform int u_frame;
uniform vec3 u_pos_ws;

layout(binding = 1) uniform sampler2D terrain_col;
layout(binding = 2) uniform sampler2D terrain_dis;
layout(binding = 3) uniform sampler2D terrain_det;
layout(binding = 4) uniform sampler2D terrain_ccol;
layout(binding = 5) uniform sampler2D terrain_cdis;
layout(binding = 6) uniform sampler2D terrain_cmat;
layout(binding = 7) uniform sampler2D terrain_acc;
layout(binding = 8) uniform sampler2D terrain_cacc;

void main()
{
  // use displacement texture here, too, so we can judge the length of edges in
  // the tessellation control shader
  vec3 p = position;
  const float k_terrain_scale = 1.0/(3.0*0.3048*2048.0);
  vec2 tex_uv = -k_terrain_scale * (u_pos_ws.xz + p.xz);
  p.y = u_terrain_bounds.x - u_pos_ws.y + u_terrain_bounds.y * texture(terrain_dis, tex_uv).r;
  gl_Position = vec4(p, 1);
}
