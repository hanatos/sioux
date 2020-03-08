#version 450 core

layout (location = 0) in vec3 position;

uniform vec2  u_terrain_bounds;
uniform float u_time;
uniform vec2  u_res;
uniform int   u_frame;
uniform vec3  u_pos_ws;

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
#if 0 // directly create vertex location
  // TODO: some coarsest grid pattern that is subdivided adaptively like:
  // | 1 | 1 | 1 |
  // +---+---+---+
  // | 2 | 4 | 2 |
  // +---+---+---+
  // | 4 | 8 | 4 |
  // or similar, and then the coarse blocks are aligned to int-rounded world
  // space position
  // TODO: need to also create index buffer or output quads here
  const float size = 4096.0f;
  const int wd = 129;
  const int j = gl_VertexID / wd;
  const int i = gl_VertexID - wd*j;
  vec3 p = vec3(i/(wd-1.0f), 0.0f, j/(wd-1.0f));
  p = vec3(-1, 0, -1) + 2.0f*p;
  float rad = max(abs(p.x), abs(p.z));
  p *= size * rad*rad;
#else
  // use vertex location from buffer
  vec3 p = position;
#endif
  // use displacement texture here, too, so we can judge the length of edges in
  // the tessellation control shader
  const float k_terrain_scale = 1.0/(3.0*0.3048*2048.0);
  vec2 tex_uv = -k_terrain_scale * (u_pos_ws.xz + p.xz);
  p.y = u_terrain_bounds.x - u_pos_ws.y + u_terrain_bounds.y * texture(terrain_dis, tex_uv).r;
  gl_Position = vec4(p, 1);
}
