#version 450 core

uniform mat4 u_mvp;
uniform mat4 u_mv;
// uniform vec3 u_pos_ws;
// uniform mat4 u_flow;
uniform float u_time;

// does not work :(
// layout(binding = 0) uniform sampler3D tex_flow;
layout(binding = 0) uniform sampler2D tex_flow;
// does work:
// layout(binding = 0, rgba16f) uniform image3D img_flow;
// layout(binding = 0, rgba16f) uniform image2D img_flow;

out vec4 color;

vec2
encrypt_tea(uvec2 arg)
{
  const uint key[] = {
    0xa341316c, 0xc8013ea4, 0xad90777d, 0x7e95761e
  };
  uint v0 = arg[0], v1 = arg[1];
  uint sum = 0;
  uint delta = 0x9e3779b9;

#pragma unroll
  for(int i = 0; i < 8; i++) {
  //for(int i = 0; i < 32; i++) {
    sum += delta;
    v0 += ((v1 << 4) + key[0]) ^ (v1 + sum) ^ ((v1 >> 5) + key[1]);
    v1 += ((v0 << 4) + key[2]) ^ (v0 + sum) ^ ((v0 >> 5) + key[3]);
  }
  return vec2(v0, v1)/4294967295.0;
}

vec3 velocity_field(vec3 x)
{
  // return vec3(0, -10, 0);
#if 0
  // vec3 tc = (x + .5) * imageSize(img_flow);
  // return imageLoad(img_flow, ivec3(tc)).rgb;
  vec3 tc = (x + .5) * 128;
  int idx = int(tc.x) + int(tc.y)*128 + int(tc.z)*128*128;
  ivec2 id = ivec2(idx - 2048*(idx/2048), idx / 2048);
  return imageLoad(img_flow, id).rgb;
#else
  vec3 tc = (x + .5);
  if(any(greaterThanEqual(tc, vec3(1.))) ||
     any(lessThanEqual(tc, vec3(0.))))
    return vec3(0.);
  // return textureLod(tex_flow, tc, 0).rgb;
  int idx = int(128*tc.x) + int(128*tc.y)*128 + int(128*tc.z)*128*128;
  ivec2 id = ivec2(idx - 2048*(idx/2048), idx / 2048);
  return texelFetch(tex_flow, id, 0).rgb;
#endif
}

float
srgb_to_linear(float srgb)
{
  if(srgb <= 0.04045f) {
    return srgb * (1.0f / 12.92f);
  }
  else {
    return pow((srgb + 0.055f) * (1.0f / 1.055f), 2.4f);
  }
}

vec3
srgb_to_linear(vec3 srgb)
{
  return vec3(
      srgb_to_linear(srgb.x),
      srgb_to_linear(srgb.y),
      srgb_to_linear(srgb.z));
}

vec3
viridis_quintic(float x)
{
  x = clamp(x, 0.0, 1.0);
  vec4 x1 = vec4(1.0, x, x * x, x * x * x); // 1 x x2 x3
  vec4 x2 = x1 * x1.w * x; // x4 x5 x6 x7
  return srgb_to_linear(vec3(
        dot(x1.xyzw, vec4(+0.280268003, -0.143510503, +2.225793877,  -14.815088879)) + dot(x2.xy, vec2(+25.212752309, -11.772589584)),
        dot(x1.xyzw, vec4(-0.002117546, +1.617109353, -1.909305070,  +2.701152864 )) + dot(x2.xy, vec2(-1.685288385,  +0.178738871 )),
        dot(x1.xyzw, vec4(+0.300805501, +2.614650302, -12.019139090, +28.933559110)) + dot(x2.xy, vec2(-33.491294770, +13.762053843))));
}

void main()
{
  int len = 2;
  uint sid = gl_VertexID / len;
  uint vid = gl_VertexID - len*sid;
  vec2 rand0 = encrypt_tea(uvec2(sid, 13371));
  vec2 rand1 = encrypt_tea(uvec2(sid, 13372));
  rand0 -= .5;
  rand1 -= .5;
  rand0 *= 0.5; // only visualise center portion
  rand1 *= 0.5;
  vec3 v = vec3(rand0.x, rand1.x, rand1.y);
  // vec3 v = vec3(rand0.x, rand0.y, 0);
  vec3 vel = velocity_field(v);
  const float dt = 0.0005f;
  const float off = 1;//mod(5*u_time, 2);
  // for(int i=0;i<vid;i++)
  int it = int(mod(5*u_time, 20));
  for(int i=0;i<it;i++)
  {
    if(i == it-1 && vid == 1) break;
    v += dt * vel;
    // if((i & 1) == 0)
    // {
    //   if(i == vid-1) v += off * dt*vel;
    //   else v += dt * vel;
    // }
    vel = velocity_field(v);
  }
  float t = length(vel)*0.02;
  color = vec4(viridis_quintic(t), 1);//t*t*t);
  // transform model to view:
  gl_Position = u_mvp * vec4(80*v, 1);
}
