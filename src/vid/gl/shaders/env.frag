#version 440
#define M_PI 3.1415926535897932384626433832795
in vec2 tex_coord;
layout(location = 0) out vec3 out_colour;
layout(location = 1) out vec2 out_motion;

layout(binding = 0) uniform sampler2D old_render;
layout(binding = 1) uniform sampler2D geo_color;
layout(binding = 2) uniform sampler2D geo_motion;
layout(binding = 3) uniform sampler2D geo_depth;

uniform vec3 u_pos;
uniform vec4 u_orient;
uniform vec2 u_terrain_bounds;
uniform float u_hfov;
uniform vec2 u_res;

//------------------------------------------------------------------------------------------
// global
//------------------------------------------------------------------------------------------

const vec3  k_sun_dir = vec3(-0.624695,0.468521,-0.624695);
const vec3  k_sun_col = vec3(1.0,0.6,0.3);
const vec3  k_fog_col = vec3(0.4,0.6,1.15);

// return transmittance of atmosphere
float atmosphere(vec3 p, vec3 w, float t)
{
  // density is d(t) = d0 * exp(-d1*height)
  const float d0 = .0001f, d1 = 0.001;
  // d0 is density at water level
  // and we want to choose b0,b1 such that height p.y + w.y*t results in right density
  // d0 * exp(-d1*(p.y+w.y*t-k_waterlevel)) = b0 * exp(-b1*t)
  const float b0 = d0 * exp(-d1*(p.y-u_terrain_bounds.x));
  const float b1 = d1*((p.y-u_terrain_bounds.x)/t + w.y);
  return exp(-b0/b1 * (1.0f - exp(-b1*t)));
}

//------------------------------------------------------------------------------------------
// sky
//------------------------------------------------------------------------------------------

vec3 renderSky( in vec3 ro, in vec3 rd )
{
  // background sky     
  vec3 col = 0.9*vec3(0.4,0.65,1.0) - rd.y*vec3(0.4,0.36,0.4);

  // sun glare    
  float sun = clamp( dot(k_sun_dir,rd), 0.0, 1.0 );
  col += 0.6*k_sun_col*pow( sun, 32.0 );

  return col;
}

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
  const float wd = tan(u_hfov*0.5);
  vec3 camray = vec3((2.0 * tex_coord.xy - 1.0) * vec2(-wd, u_res.y*wd/u_res.x), 1);
  vec3 w = normalize(quat_transform(camray, u_orient));

  //  bound by cockpit texture and geo_depth from below!
  // somehow the sky and depth buffers are unhappy with > tmax:
  vec2 bounds = vec2(1.0f, 10000.0f);

  vec4 env_sample = vec4(renderSky(u_pos, w), bounds.y);

  //------------------------------------------------
  // raster geo sample
  //------------------------------------------------
  vec4 geo_sample = textureLod(geo_color, tex_coord, 0);
  vec2 geo_mot    = texture(geo_motion, tex_coord).rg;

  float depth = texture(geo_depth, tex_coord).x;
  const float n = .10, f = 10000.0;
  float d2 = 2.0f * depth - 1.0f;
  d2 = 2.0f * n*f/(f + n - d2* (f-n));
  const float z = length(vec3(w.xy * d2, d2));
  if(env_sample.w > z)
    env_sample = vec4(mix(env_sample.xyz, geo_sample.xyz, geo_sample.w), env_sample.w);

  vec3 col  = env_sample.rgb;
  float dist = min(z, env_sample.w);
  col += (1.0-atmosphere(u_pos, w, dist))*k_fog_col;

  //------------------------------------------------
  // final
  //------------------------------------------------

  // sun glare (TODO: only if sun is visible at all?)
  float sun = clamp( dot(k_sun_dir,w), 0.0, 1.0 );
  col += 0.25*vec3(1.0,0.4,0.2)*pow( sun, 4.0 );

  //------------------------------------------------
  // gamma and colour grading
  //------------------------------------------------
  col = clamp(col, vec3(0), vec3(1));

  // gamma
  col = sqrt(col);

#if 0
  col = col*0.15 + 0.85*col*col*(3.0-2.0*col);            // contrast
  col = pow( col, vec3(1.0,0.92,1.0) );   // soft green
  col *= vec3(1.02,0.99,0.99);            // tint red
  col.z = (col.z+0.1)/1.1;                // bias blue
  col = mix( col, col.yyy, 0.15 );        // desaturate

  col = clamp( col, 0.0, 1.0 );
#endif

  out_colour = col;
  out_motion = geo_mot;
}
