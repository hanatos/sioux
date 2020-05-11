#version 440
#extension GL_ARB_bindless_texture : require
// #extension GL_ARB_gpu_shader_int64 :enable

#define M_PI 3.1415926535897932384626433832795
in vec3 shading_normal;
in vec3 position_cs;
in vec4 old_pos4;
in vec2 tex_uv;
in flat uvec2 id;
uniform vec2 u_terrain_bounds;
uniform vec3 u_sun; // direction towards sun
uniform float u_lod;
uniform float u_hfov;
uniform int u_frame;
uniform vec3 u_pos_ws;
layout(binding = 0) uniform sampler2D render;

struct material_t
{
  // uint64_t tex_handle;
  uvec2 tex_handle;
  uint dunno0;
  uint dunno1;
  uint dunno2;
  uint dunno3;
};
layout(std430,binding=1) buffer geo_material
{
  material_t mat[];
};
layout(std430,binding=2) buffer geo_anim
{
  uint b_anim[];
};
layout(location = 0) out vec4 out_colour;
layout(location = 1) out vec2 out_motion; // output old fragment position

// layout(std140, binding = 0) uniform frameBuffer {
//     // Here are all frameBuffer's renderTarget
//     sampler2D gBufferNormal; // uint64_t from host
// };
// layout(bindless_sampler) uniform sampler2D bindless;

void
encrypt_tea(inout uvec2 arg)
{
  const uint key[] = {
    0xa341316c, 0xc8013ea4, 0xad90777d, 0x7e95761e
  };
  uint v0 = arg[0], v1 = arg[1];
  uint sum = 0;
  uint delta = 0x9e3779b9;

  #pragma unroll
  for(int i = 0; i < 8; i++) {
    sum += delta;
    v0 += ((v1 << 4) + key[0]) ^ (v1 + sum) ^ ((v1 >> 5) + key[1]);
    v1 += ((v0 << 4) + key[2]) ^ (v0 + sum) ^ ((v0 >> 5) + key[3]);
  }
  arg[0] = v0;
  arg[1] = v1;
}

const vec3 k_fog_col = vec3(0.4,0.6,1.15);
// return transmittance of atmosphere
float atmosphere(vec3 p, vec3 w, float t)
{
  const float d0 = .08, d1 = 0.8;
  const float b0 = d0 * exp(-d1*(p.y-u_terrain_bounds.x));
  const float b1 = d1*((p.y-u_terrain_bounds.x)/t + w.y);
  return exp(-b0/b1 * (1.0f - exp(-b1*t)));
}

// TODO: need to depend on u_lod, higher res textures make materials look more shiny otherwise!
vec4 diffenv(vec3 w, int lod)
{
  const float hfov = u_hfov;
  const float vfov = 9./16.0 * hfov;

  // get tex coord that corresponds best to world space ray:
  float angle_y = asin(w.y);
  float cosy = sqrt(max(0.0, 1.0-w.y*w.y));
  float angle_x = atan(w.x, w.z);
  vec2 tex_coord = vec2(angle_x, angle_y)/vec2(hfov,vfov);
  if(u_lod > 1)
  {
    float tc1 = length(tex_coord);
    float tc2 = (sqrt((4*u_lod*u_lod-4*u_lod)*tc1+1)-1)/(2*u_lod-2);
    tex_coord *= tc2/tc1;
  }
  // tex_coord = clamp((tex_coord + 1.0)*.5f, 0, 1);
  tex_coord = (tex_coord + 1.0)*.5f;
  tex_coord = abs(tex_coord);
  if(tex_coord.x > 1) tex_coord.x = 2 - tex_coord.x;
  if(tex_coord.y > 1) tex_coord.y = 2 - tex_coord.y;
  // if(tex_coord.x < 0) return vec4(vec3(0), 1);
  // if(tex_coord.y < 0) return vec4(vec3(0), 1);
  // if(tex_coord.x > 1) return vec4(vec3(0), 1);
  // if(tex_coord.y > 1) return vec4(vec3(0), 1);
  // ivec2 itex = ivec2(textureSize(render, 3) * tex_coord);
  // return texelFetch(render, itex, lod);
  // return vec4(textureLod(render, tex_coord, lod).rgb, 1);
  return textureLod(render, tex_coord, lod);
}

float fake_fresnel(float r, float doth)
{
  // schlick:
  return r + (1.0-r)*pow(1-doth, 5);
}

//===============================================================================
// brdf entirely stolen from jonathan:
float erfc(float x) {
  return 2.0 * exp(-x * x) / (2.319 * x + sqrt(4.0 + 1.52 * x * x));
}

float erf(float x) {
  float a  = 0.140012;
  float x2 = x*x;
  float ax2 = a*x2;
  return sign(x) * sqrt( 1.0 - exp(-x2*(4.0/M_PI + ax2)/(1.0 + ax2)) );
}

float Lambda(float cosTheta, float sigmaSq) {
  float v = cosTheta / sqrt((1.0 - cosTheta * cosTheta) * (2.0 * sigmaSq));
  return max(0.0, (exp(-v * v) - v * sqrt(M_PI) * erfc(v)) / (2.0 * v * sqrt(M_PI)));
  //return (exp(-v * v)) / (2.0 * v * sqrt(M_PI)); // approximate, faster formula
}

// FIXME: seem to have the normal distribution upside down somehow
// L, V, N, Tx, Ty in world space
float brdf(vec3 L, vec3 V, vec3 N, vec3 Tx, vec3 Ty, vec2 sigmaSq)
{
#if 1 // ???
  return pow(max(0.0, dot(L, reflect(-V, N))), 10);
#endif
  vec3 H = normalize(L + V);
  float zetax = dot(H, Tx) / dot(H, N);
  float zetay = dot(H, Ty) / dot(H, N);

  float zL = dot(L, N); // cos of source zenith angle
  float zV = dot(V, N); // cos of receiver zenith angle
  float zH = dot(H, N); // cos of facet normal zenith angle
  if(zL < 0 || zV < 0 || zH < 0) return 0.0;
  float zH2 = zH * zH;

  float p = exp(-0.5 * (zetax * zetax / sigmaSq.x + zetay * zetay / sigmaSq.y))
    / (2.0 * M_PI * sqrt(sigmaSq.x * sigmaSq.y));

  float tanV = atan(dot(V, Ty), dot(V, Tx));
  float cosV2 = 1.0 / (1.0 + tanV * tanV);
  float sigmaV2 = sigmaSq.x * cosV2 + sigmaSq.y * (1.0 - cosV2);

  float tanL = atan(dot(L, Ty), dot(L, Tx));
  float cosL2 = 1.0 / (1.0 + tanL * tanL);
  float sigmaL2 = sigmaSq.x * cosL2 + sigmaSq.y * (1.0 - cosL2);

  float fresnel = 0.02 + 0.98 * pow(1.0 - dot(V, H), 5.0);

  zL = max(zL, 0.01);
  zV = max(zV, 0.01);

  return fresnel * p / ((1.0 + Lambda(zL, sigmaL2) + Lambda(zV, sigmaV2)) * zV * zH2 * zH2 * 4.0);
}
//===============================================================================


void main()
{
  // XXX stupid hack because normals are flipped for helicopter
  // const vec3 n = (u_rotate == 2? -1 : 1)*normalize(shading_normal);
  const vec3 n = normalize(shading_normal);
#if 0 // debug
  // if(position_cs.y > -5) discard;
  // visualise:
  out_colour = vec4((1.0+n*vec3(1,1,-1))*.5, 1.0); // normals
  // out_colour = vec4(tex_uv.zzz/8, 1.0); // uvs
  // frag_color = vec4(position_cs.zzz*1, 1.0);
  // frag_color = vec4(position_cs, 1.0); // positions: +x: right, +y: up, +z: front
  // => normals should be red right, green top, dim yellow in front (0,0,-z) => (.5, .5, 0)
  // bottom: red, left: blue/green, right: orange
  // top  : (0,1,0) => (.5,1,.5) bright green
  // right: (1,0,0) =>
  // frag_color = vec4(u_sun, 1.0); // sun pos
  return;
#endif

  vec3 wo = vec3(0, 1, 0);// XXX u_sun; // outgoing towards sun
  vec3 wi = normalize(position_cs); // incoming from (0,0,0)
    // debug mip levels:
    // vec3 rx = reflect(wi, n);
    // frag_color = vec4(diffenv(vec3(1,1,-1)*n, 1).rgb, 1);
    // frag_color = vec4(diffenv(rx, 6).rgb, 1);
    // return;//XXX
  // const float cos_wo = dot(wo, n);

  vec3 col = vec3(0);
  vec4 diffcol = vec4(0.0, 0, 0, 1);
  uint matb = id.x;
  uint inst = id.y;

  // highlight from direct sun:
  // TODO: shadowing! (pass in as uniform?)
  const vec3 Tx = cross(n, vec3(0, 0, -1));
  const vec3 Ty = cross(n, Tx);
  const vec3 L_sun = 5 * vec3(1.0, .6, .3);
  const vec2 r = vec2(0.1, 0.1); // roughness
  const float fr = brdf(-wi, wo, n, Tx, Ty, r);
  col += fr * L_sun;
  col += vec3(pow((n.y+1)/2.0, 2));
  // col = vec3(pow(abs(dot(-wi, n)), 2));
  if((mat[matb].dunno0>>24) > 0) // explosions etc with static light
    col = vec3(1);

  // const vec3 ref = reflect(wi, n);

  uint anim = (mat[matb].dunno3>>8) & 0xffu;
  if(anim > 0) matb = matb + ((b_anim[id.y]) % anim);
  // uint64_t th = mat[matb].tex_handle;
  uvec2 th = mat[matb].tex_handle;
  if(th.x != -1)
    diffcol = texture(sampler2D(th), tex_uv.xy);
  else
  { // no texture
    uint trans = (mat[matb].dunno2>>24) & 0xffu;
    if(trans > 0)
    { // window glass
      diffcol = vec4(0.9, 0.4, 0.1, 0.2);
      if(dot(-wi, n) < 0) discard; // hide backfacing
    }
    // radar nose colour:
    else diffcol.rgb = unpackUnorm4x8(mat[matb].dunno1).rgb;
  }
  // if((mat[matb].dunno0>>24) > 0)
  {
    // fake indirect diffuse
    const vec3 L_sky = vec3(0.5);// XXX  * diffenv(ref, 8).rgb;
    const float cos_wi = max(0.0, -dot(wi, n));
    col *= diffcol.rgb;//L_sky * cos_wi * diffcol;
  }

  // stochastic alpha
  uvec2 seed = uvec2(u_frame, 1337*gl_FragCoord.x+19937*gl_FragCoord.y);//uint(1337*tex_uv.x + 19937*(tex_uv.y+1)));
  encrypt_tea(seed);
  if(float(seed.x) / 4294967296.0 > diffcol.w) discard;
  diffcol.w = 1.0;

  // fake reflections
  // col += fake_fresnel(0.01, cos_wi) * vec3(1);//diffenv(ref, 2).rgb;

  out_colour.rgb = col;//tau * col + (1-tau)*k_fog_col;
  out_colour.w = diffcol.w;

  // output motion vector:
  // out_motion = gl_FragCoord.xy - old_pos4.xy / old_pos4.w;
  out_motion = old_pos4.xy / old_pos4.w;
}

