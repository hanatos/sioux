#version 450 core

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

out vec4 frag_color;
in vec3 pos_ws;
in vec3 normal;

void main()
{
  // frag_color = vec4(0.1f, 1.0f, 1.0f, 1.0f);

  // XXX TODO: get normal from precomputed normal of texture map in 3 channels
  const vec3 sun = vec3(1.0, 1.0, 0.0);
  float diff = .5 + .5*max(0, dot(sun, normalize(normal)));

  const float k_terrain_scale = 1.0/(3.0*0.3048*2048.0);
  vec2 tex_uv = -k_terrain_scale * (u_pos_ws.xz + pos_ws.xz);
  frag_color = diff*texture(terrain_col, tex_uv);

  // frag_color = vec4(0.0f, mod(100*tc, 1), 1.0f);
  // frag_color = vec4(0.0f, 0.1*mod(tex_uv3, 1), 1.0f);
}
