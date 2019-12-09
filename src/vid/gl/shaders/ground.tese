#version 450 core
layout (triangles, equal_spacing, cw) in;

uniform vec2 u_terrain_bounds;
uniform float u_time;
uniform vec2 u_res;
uniform int u_frame;
uniform vec3 u_pos_ws;
uniform mat4 u_mvp;
uniform mat4 u_mv;

out vec3 te_pos_ws;

layout(binding = 1) uniform sampler2D terrain_col;
layout(binding = 2) uniform sampler2D terrain_dis;
layout(binding = 3) uniform sampler2D terrain_det;
layout(binding = 4) uniform sampler2D terrain_ccol;
layout(binding = 5) uniform sampler2D terrain_cdis;
layout(binding = 6) uniform sampler2D terrain_cmat;
layout(binding = 7) uniform sampler2D terrain_acc;
layout(binding = 8) uniform sampler2D terrain_cacc;

float hash1( float n )
{
  return fract( n*17.0*fract( n*0.3183099 ) );
}

float get_height(vec3 pos)
{
  const float k_terrain_scale = 1.0/(3.0*0.3048*2048.0);
  vec2 uv = -k_terrain_scale * (u_pos_ws.xz + pos.xz);
  float dist = length(pos);
  int lod = 0;
  lod = clamp(int(dist / 100), 0, 5);
  float h = textureLod(terrain_dis, uv, lod).r;
  if(dist > 100) return h;
  // return h;
#if 1
  // float mat = 256*texture(terrain_det, uv).r;
  // uvec3 mat = texelFetch(terrain_det, ivec2(1024*uv+.5f), 0).rgb/4;
  // uvec3 mat = textureLod(terrain_det, uv, 0).rgb/4;
  uvec3 mat = uvec3(textureLod(terrain_det, uv, 0).rgb*256.0/4.0);
  uint tile = mat.r + 72;//144;
  if(mat.r == 0) tile = mat.b;
#if 1
  float dis = texelFetch(
    terrain_cdis, ivec2(mod(16*1024*uv, vec2(16)))+ivec2(0, 16*tile), 0).r;
#else
  float dis = textureLod(
      terrain_cdis, (ivec2(mod(16*1024*uv, vec2(16)))+ivec2(0, 16*tile))/vec2(16.0, 4096.0), 0).r;
#endif
  // if(mat.r != 0) return h;// + (dis-.5)/64.0;
  // if(mat.r == 0) return h + 0.01;
  return h + (.5-dis)/32.0;//64.0;
#endif
}

void main(void)
{ 
  vec4 p = // input model coordinates
        gl_TessCoord.x*gl_in[0].gl_Position
      + gl_TessCoord.y*gl_in[1].gl_Position
      + gl_TessCoord.z*gl_in[2].gl_Position;

  p.y = u_terrain_bounds.x - u_pos_ws.y + u_terrain_bounds.y * get_height(vec3(p));
  gl_Position = u_mvp * p; //P*vec4(v, 1) + vec4(jitter, 0, 0);
  // old_pos4 = P*vec4(ov, 1) + vec4(jitter, 0, 0);
  te_pos_ws = vec3(p);
}
