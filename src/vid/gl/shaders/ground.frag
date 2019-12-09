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

vec4 get_colour(vec3 pos)
{
  const float k_terrain_scale = 1.0/(3.0*0.3048*2048.0);
  vec2 uv = -k_terrain_scale * (u_pos_ws.xz + pos.xz);
#if 0
  return texture(terrain_col, uv);
#else
  // float mat = 256*texture(terrain_det, uv).r;
  // ivec3 mat = ivec3(256*texelFetch(terrain_det, ivec2(1024*uv+.5f), 0).rgb);
  vec3 base = texture(terrain_col, uv).rgb;

  if(length(pos) > 100) return vec4(base, 1);
  // uvec3 mat = texelFetch(terrain_det, ivec2(1024*uv+.5f), 0).rgb/4;
  uvec3 mat = uvec3(textureLod(terrain_det, uv, 0).rgb*256.0/4.0);
  // return vec4(mat, 1);
  // mat.y += int(mod(2.0*u_time, 16));
  // red  : some tile index 127 .. 255, uniform structure
  // green: some tile index < 128, can see pathways and different levels, can sometimes see isolines
  // blue : marks parts that are masked out in red and green, looks like stones/pebbles
  //  (green mask seems to be bigger, blue and red maybe complements)
  // mat.rgb /= 4; // last two bits are apparently fixed at 11??
  // r 31..63, g 0..31
  // map 31 or 63 to 0
  // got tiles from 0 to 255
  // int tile = 63-mat.r;//
  // int tile = -31+mat.r;//192+mat.r;//4*mat.g + 128;
  // int tile = -63+mat.r;
#if 1
  uint tile = mat.r + 72;//144;
  if(mat.r == 0) tile = mat.b;
  vec4 col = texelFetch(
          terrain_ccol, (ivec2(mod(16*1024*vec2(uv.x, uv.y), vec2(16)))+ivec2(0, 16*tile)), 0);
  // vec4 col = textureLod(
  //         terrain_ccol, (ivec2(mod(16*1024*uv, vec2(16)))+ivec2(0, 16*mat.y))/vec2(16.0, 1024), 0);
  return vec4(mix(base, col.rgb, col.a), 1);
  return vec4(vec3(mat)/256.0, 1);
  return vec4(base, 1);
  return vec4(col.rgb, 1);
#endif
#endif

  //     ivec3 mat = ivec3(mod(
  //           // int(u_time)
  //           // 128
  //           255-64-ivec3(256*texture(terrain_det, tc).rgb), 256));
  //     vec4 ccol = texelFetch(
  //         terrain_ccol, ivec2(mod(16*1024*tc, 16))+ivec2(0, 16*mat.b), 0);
  //     vec4 cmat = texelFetch(
  //         terrain_cmat, ivec2(mod(16*1024*tc, 16))+ivec2(0, 16*mat.x), 0);
}

void main()
{
  // frag_color = vec4(0.1f, 1.0f, 1.0f, 1.0f);

  // XXX TODO: get normal from precomputed normal of texture map in 3 channels
  const vec3 sun = normalize(vec3(1.0, 1.0, 0.0));
  float diff = 0.98f + 0.02f*max(0, dot(sun, normalize(normal)));
  frag_color = diff*get_colour(pos_ws);

  // frag_color = vec4(0.0f, mod(100*tc, 1), 1.0f);
  // frag_color = vec4(0.0f, 0.1*mod(tex_uv3, 1), 1.0f);
}
