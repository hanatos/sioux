#version 440
#define M_PI 3.1415926535897932384626433832795
in vec2 tex_coord;
layout(binding = 0) uniform sampler2D cube_L_col;
layout(binding = 1) uniform sampler2D cube_L_mot;
layout(binding = 2) uniform sampler2D cube_R_col;
layout(binding = 3) uniform sampler2D cube_R_mot;
layout(binding = 4) uniform sampler2D cube_L_depth;
layout(binding = 5) uniform sampler2D cube_R_depth;
uniform float u_time;
uniform float u_lod;
uniform float u_hfov;
uniform float u_vfov;
uniform vec3 u_pos;
uniform vec4 u_orient;
layout(location = 0) out vec4 out_colour;
layout(location = 1) out vec3 out_motion;

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

#if 1
vec4 sample_catmull_rom( sampler2D tex, vec2 uv )
{
  // We're going to sample a a 4x4 grid of texels surrounding the target UV coordinate. We'll do this by rounding
  // down the sample location to get the exact center of our "starting" texel. The starting texel will be at
  // location [1, 1] in the grid, where [0, 0] is the top left corner.
  vec2 texSize = textureSize(tex, 0);
  vec2 samplePos = uv * texSize;
  vec2 texPos1 = floor(samplePos - 0.5) + 0.5;

  // Compute the fractional offset from our starting texel to our original sample location, which we'll
  // feed into the Catmull-Rom spline function to get our filter weights.
  vec2 f = samplePos - texPos1;

  // Compute the Catmull-Rom weights using the fractional offset that we calculated earlier.
  // These equations are pre-expanded based on our knowledge of where the texels will be located,
  // which lets us avoid having to evaluate a piece-wise function.
  vec2 w0 = f * ( -0.5 + f * (1.0 - 0.5*f));
  vec2 w1 = 1.0 + f * f * (-2.5 + 1.5*f);
  vec2 w2 = f * ( 0.5 + f * (2.0 - 1.5*f) );
  vec2 w3 = f * f * (-0.5 + 0.5 * f);

  // Work out weighting factors and sampling offsets that will let us use bilinear filtering to
  // simultaneously evaluate the middle 2 samples from the 4x4 grid.
  vec2 w12 = w1 + w2;
  vec2 offset12 = w2 / (w1 + w2);

  // Compute the final UV coordinates we'll use for sampling the texture
  vec2 texPos0 = texPos1 - vec2(1.0);
  vec2 texPos3 = texPos1 + vec2(2.0);
  vec2 texPos12 = texPos1 + offset12;

  texPos0 /= texSize;
  texPos3 /= texSize;
  texPos12 /= texSize;

  vec4 result = vec4(0.0);
  result += textureLod(tex, vec2(texPos0.x,  texPos0.y),  0) * w0.x * w0.y;
  result += textureLod(tex, vec2(texPos12.x, texPos0.y),  0) * w12.x * w0.y;
  result += textureLod(tex, vec2(texPos3.x,  texPos0.y),  0) * w3.x * w0.y;

  result += textureLod(tex, vec2(texPos0.x,  texPos12.y), 0) * w0.x * w12.y;
  result += textureLod(tex, vec2(texPos12.x, texPos12.y), 0) * w12.x * w12.y;
  result += textureLod(tex, vec2(texPos3.x,  texPos12.y), 0) * w3.x * w12.y;

  result += textureLod(tex, vec2(texPos0.x,  texPos3.y),  0) * w0.x * w3.y;
  result += textureLod(tex, vec2(texPos12.x, texPos3.y),  0) * w12.x * w3.y;
  result += textureLod(tex, vec2(texPos3.x,  texPos3.y),  0) * w3.x * w3.y;

  return result;
}
#endif

vec3 cube_tc_from_ray(vec3 camray, bool X)
{
  const float s = sin(u_hfov*0.25);
  const float c = cos(u_hfov*0.25);
  mat3 R = mat3( c, 0.0,  s,   0,  1,   0, -s,  0,  c);
  mat3 L = mat3( c, 0.0, -s,   0,  1,   0,  s,  0,  c);
  vec3 v;
  if(X) v = R*camray;
  else  v = L*camray;
  v.xy /= v.z;
  const float r = s/c, l = -r;
  const float t = 1.38*tan(u_vfov*0.5),  b = -t; // add 38% security margin
  vec2 tc = vec2(
      (v.x-l)/(r-l),
      (v.y-b)/(t-b));
  float depth;
  if(X) depth = texture(cube_R_depth, tc).x;
  else  depth = texture(cube_L_depth, tc).x;
  const float n = .10, f = 10000.0;
  float d2 = 2.0f * depth - 1.0f;
  d2 = 2.0f * n*f/(f + n - d2* (f-n));
  const float z = length(vec3(v.xy * d2, d2));
  return vec3(tc, z);
}

void main()
{
  const float n = .10, f = 10000.0;
  // compute world space ray direction
  float angle_x = u_hfov * (tex_coord.x - 0.5);
  float angle_y = u_vfov * (tex_coord.y - 0.5);
  // equi-angular:
  vec3 camray = vec3(sin(angle_x)*cos(angle_y), sin(angle_y), cos(angle_x)*cos(angle_y));

  vec4 geo_col = vec4(0);
  float geo_depth = 100000.0;
  // XXX FIXME: compute motion vectors correctly!
  vec2 geo_mot = vec2(0);
  // blit cube maps
  // version without motion vectors
  if(tex_coord.x > .5)
  {
    const vec3 tc = cube_tc_from_ray(camray, true);
    geo_col = texture(cube_R_col, tc.xy);
    geo_depth = tc.z;
  }
  else
  {
    const vec3 tc = cube_tc_from_ray(camray, false);
    geo_col = texture(cube_L_col, tc.xy);
    // geo_col = sample_catmull_rom(cube_L_col, tc.xy);
    geo_depth = tc.z;
  }

  out_colour = geo_col;
  // float depth = 2.0f * geo_depth - 1.0f;
  out_motion = vec3(geo_mot, geo_depth);//2.0f * n*f/(f + n - depth * (f-n)));
}
