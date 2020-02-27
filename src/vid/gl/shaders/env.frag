#version 440
#extension GL_ARB_bindless_texture : require
#define M_PI 3.1415926535897932384626433832795
in vec2 tex_coord;
layout(location = 0) out vec3 out_colour;
layout(location = 1) out vec2 out_motion;

layout(binding = 0) uniform sampler2D old_render;
layout(binding = 1) uniform sampler2D geo_color;
layout(binding = 2) uniform sampler2D geo_motion;
layout(binding = 3) uniform sampler2D geo_depth;

layout(bindless_sampler) uniform sampler2D img_noise0;
layout(bindless_sampler) uniform sampler2D img_noise1;

uniform mat4 u_mvp_old;
uniform vec3 u_pos;
uniform vec4 u_orient;
uniform vec2 u_terrain_bounds;
uniform float u_hfov;
uniform float u_time;
uniform vec2 u_res;

//------------------------------------------------------------------------------------------
// global
//------------------------------------------------------------------------------------------

// TODO: get from global uniform buffer object (and read in from .mis file)
const vec3  k_sun_dir = vec3(-0.624695,0.468521,-0.624695);
const vec3  k_sun_col = vec3(1.0,0.7,0.4);
const vec3  k_fog_col = vec3(.75, .65, .5);//vec3(0.5, 0.6, 0.7);//vec3(0.4,0.6,1.15);
const vec3  k_cloud_speed = 20*vec3(1, 0, 1);//vec3(5, 190, 0);

float hash1( float n )
{
  return fract( n*17.0*fract( n*0.3183099 ) );
}

#if 1
//------------------------------------------------------------------------------------------
// clouds stolen from
// Himalayas. Created by Reinder Nijhoff 2018
// @reindernijhoff
//
// https://www.shadertoy.com/view/MdGfzh
//------------------------------------------------------------------------------------------
#define CLOUD_MARCH_STEPS 12
#define CLOUD_SELF_SHADOW_STEPS 6

#define EARTH_RADIUS     20000.    //(1500000.) // (6371000.)
#define CLOUDS_BOTTOM   (450.)
#define CLOUDS_TOP      (750.)

#define CLOUDS_COVERAGE (.37)

#define CLOUDS_DETAIL_STRENGTH (.4)
#define CLOUDS_BASE_EDGE_SOFTNESS (.1)
#define CLOUDS_BOTTOM_SOFTNESS (.25)
#define CLOUDS_DENSITY (.03)
#define CLOUDS_SHADOW_MARGE_STEP_SIZE (10.)
#define CLOUDS_SHADOW_MARGE_STEP_MULTIPLY (1.3)
#define CLOUDS_FORWARD_SCATTERING_G (.8)
#define CLOUDS_BACKWARD_SCATTERING_G (-.2)
#define CLOUDS_SCATTERING_LERP (.5)

#define CLOUDS_AMBIENT_COLOR_TOP (vec3(149., 167., 200.)*(1.5/255.))
#define CLOUDS_AMBIENT_COLOR_BOTTOM (vec3(39., 67., 87.)*(1.5/255.))
#define CLOUDS_MIN_TRANSMITTANCE .1

#define CLOUDS_BASE_SCALE 1.51
#define CLOUDS_DETAIL_SCALE 20.

//
// Cloud shape modelling and rendering 
//
float HenyeyGreenstein( float sundotrd, float g) {
	float gg = g * g;
	return (1. - gg) / pow( 1. + gg - 2. * g * sundotrd, 1.5);
}

// center at zero
float intersect_sphere(vec3 ro, vec3 rd, float r)
{
  const float a = 1.;//dotproduct(ray->dir, ray->dir);
  // float o[3] = {ray->pos[0]-center[0],ray->pos[1]-center[1],ray->pos[2]-center[2]};
  const float b = 2.0*dot(ro, rd);
  const float c = dot(ro, ro) - r*r;

  float discrim = b*b - 4.0*a*c;
  if (discrim < 0) return -1.f;

  float temp, sqrt_discrim = sqrt(discrim);
  if (b < 0)
    temp = -0.5 * (b - sqrt_discrim);
  else
    temp = -0.5 * (b + sqrt_discrim);

  const float x0 = temp / a;
  const float x1 = c / temp;
  return x0 <= 0.0 ? x1 : (x1 <= 0.0 ? x0 : min(x0, x1));
}

float linearstep( const float s, const float e, float v ) {
    return clamp( (v-s)*(1./(e-s)), 0., 1. );
}

float linearstep0( const float e, float v ) {
    return min( v*(1./e), 1. );
}

float remap(float v, float s, float e) {
	return (v - s) / (e - s);
}

float cloud_map_base(vec3 p, float norY)
{
  vec2 uv = p.xz * 0.0003 * CLOUDS_BASE_SCALE;
  vec3 cloud = textureLod(img_noise0, uv, 0).rgb;

  float n = norY*norY;
  n *= cloud.b ;
  n+= pow(1.-norY, 16.); 
  return remap( cloud.r - n, cloud.g, 1.);
}

float cloud_map_detail(vec3 p)
{
  // 3d lookup in 2d texture :(
  p = abs(p) * (0.003 * CLOUDS_BASE_SCALE * CLOUDS_DETAIL_SCALE);

  float yi = mod(p.y,32.);
  ivec2 offset = ivec2(mod(yi,8.), mod(floor(yi/8.),4.))*34 + 1;
  float a = texture(img_noise1, (mod(p.xz,32.)+vec2(offset.xy)+1.)/textureSize(img_noise1, 0).xy).r;

  yi = mod(p.y+1.,32.);
  offset = ivec2(mod(yi,8.), mod(floor(yi/8.),4.))*34 + 1;
  float b = texture(img_noise1, (mod(p.xz,32.)+vec2(offset.xy)+1.)/textureSize(img_noise1, 0).xy).r;

  return mix(a,b,fract(p.y));
}

float cloud_gradient( float norY ) {
    return linearstep( 0., .05, norY ) - linearstep( .8, 1.2, norY);
}

float cloud_map(vec3 pos, float norY)
{
  vec3 ps = pos;

  float m = cloud_map_base(ps+k_cloud_speed*u_time, norY);
  m *= cloud_gradient( norY );

  float dstrength = smoothstep(1., 0.5, m);

  // erode with detail
  if(dstrength > 0.)
    m -= cloud_map_detail(ps) * dstrength * CLOUDS_DETAIL_STRENGTH;

  m = smoothstep( 0., CLOUDS_BASE_EDGE_SOFTNESS, m+(CLOUDS_COVERAGE-1.) );
  m *= linearstep0(CLOUDS_BOTTOM_SOFTNESS, norY);

  return clamp(m * CLOUDS_DENSITY * (1.+max((ps.x-7000.)*0.005,0.)), 0., 1.);
}

float volumetric_shadow(in vec3 from, vec3 off)
{
  float dd = CLOUDS_SHADOW_MARGE_STEP_SIZE;
  vec3 rd = k_sun_dir;
  float d = dd * .5;
  float shadow = 1.0;

  for(int s=0; s<CLOUD_SELF_SHADOW_STEPS; s++)
  {
    vec3 pos = from + rd * d;
    float norY = (length(pos) - (EARTH_RADIUS + CLOUDS_BOTTOM)) * (1./(CLOUDS_TOP - CLOUDS_BOTTOM));

    if(norY > 1.) return shadow;

    float muE = cloud_map(pos + off, norY);
    shadow *= exp(-muE * dd);

    dd *= CLOUDS_SHADOW_MARGE_STEP_MULTIPLY;
    d += dd;
  }
  return shadow;
}

// tiny encryption algorithm random numbers
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
    sum += delta;
    v0 += ((v1 << 4) + key[0]) ^ (v1 + sum) ^ ((v1 >> 5) + key[1]);
    v1 += ((v0 << 4) + key[2]) ^ (v0 + sum) ^ ((v0 >> 5) + key[3]);
  }
  // 32 bit
  // return .00000000023283064365 * vec2(v0, v1);
  // 16 bit
  return .00001525878906250000 * vec2(v0>>16, v1>>16);
}

vec2 next_rand(inout uvec2 seed)
{
  seed.y++;
  return encrypt_tea(seed);
}

float ground_shadow(vec3 ro, inout uvec2 seed)
{
  if(ro.y > CLOUDS_BOTTOM) return 1.0;
  ro.y = ro.y + EARTH_RADIUS;
  const vec3 off = vec3(u_pos.x, 0.0, u_pos.z);
  ro.xz -= u_pos.xz;

  float beg = intersect_sphere(ro, k_sun_dir, EARTH_RADIUS + CLOUDS_BOTTOM);
  float end = intersect_sphere(ro, k_sun_dir, EARTH_RADIUS + CLOUDS_TOP);
  int steps = CLOUD_SELF_SHADOW_STEPS;
  float dt = (end - beg)/steps;
  float d = beg;
  d += next_rand(seed).x * dt;
  float trans = 0.0;
  for(int s=0;s<steps;s++)
  {
    vec3 p = ro + d * k_sun_dir;
    float norY = clamp( (length(p) - (EARTH_RADIUS + CLOUDS_BOTTOM)) * (1./(CLOUDS_TOP - CLOUDS_BOTTOM)), 0., 1.);
    float alpha = .35*cloud_map(ro+off, norY);
    trans -= alpha * dt;
    d += dt;
  }
  return exp(trans);
}

vec4 render_clouds(vec3 ro, vec3 rd, inout float dist, inout uvec2 seed)
{
  ro.y = ro.y + EARTH_RADIUS;
  const vec3 off = vec3(u_pos.x, 0.0, u_pos.z);
  ro.xz -= u_pos.xz;

  float bot = intersect_sphere(ro, rd, EARTH_RADIUS + CLOUDS_BOTTOM);
  float top = intersect_sphere(ro, rd, EARTH_RADIUS + CLOUDS_TOP);
  if(top < 0) return vec4(0,0,0,1); // out completely

  float start, end;
  // consider various valid cases:
  if(ro.y < EARTH_RADIUS + CLOUDS_BOTTOM)
  {
    start = bot;
    end   = top;
  }
  else if(ro.y < EARTH_RADIUS + CLOUDS_TOP)
  {
    start = 0;
    end = max(bot, top);
  }
  else // if(ro.y >= EARTH_RADIUS + CLOUDS_TOP)
  {
    start = top;
    end   = bot > 0.0 ? bot : .4*EARTH_RADIUS;
  }

  if (start > dist)
    return vec4(0,0,0,1);

  end = min(end, dist);

  float sundotrd = dot( rd, -k_sun_dir);

  // raymarch
  float d = start;
  float dD = (end-start) / float(CLOUD_MARCH_STEPS);

  float h = next_rand(seed).x;
  d -= dD * h;

  float scattering =  mix( HenyeyGreenstein(sundotrd, CLOUDS_FORWARD_SCATTERING_G),
      HenyeyGreenstein(sundotrd, CLOUDS_BACKWARD_SCATTERING_G), CLOUDS_SCATTERING_LERP );

  float transmittance = 1.0;
  vec3 scatteredLight = vec3(0.0, 0.0, 0.0);

  dist = EARTH_RADIUS;

  for(int s=0; s<CLOUD_MARCH_STEPS; s++)
  {
    vec3 p = ro + d * rd;

    float norY = clamp( (length(p) - (EARTH_RADIUS + CLOUDS_BOTTOM)) * (1./(CLOUDS_TOP - CLOUDS_BOTTOM)), 0., 1.);

    float alpha = cloud_map(p+off, norY);

    if( alpha > 0. )
    {
      dist = min( dist, d);
      vec3 ambientLight = mix( CLOUDS_AMBIENT_COLOR_BOTTOM, CLOUDS_AMBIENT_COLOR_TOP, norY );

      vec3 S = (ambientLight + k_sun_col * (scattering * volumetric_shadow(p, off))) * alpha;
      float dTrans = exp(-alpha * dD);
      vec3 Sint = (S - S * dTrans) * (1. / alpha);
      scatteredLight += transmittance * Sint; 
      transmittance *= dTrans;
    }

    if( transmittance <= CLOUDS_MIN_TRANSMITTANCE ) break;

    d += dD;
  }

  return vec4(scatteredLight, transmittance);
}

#endif


// return transmittance of atmosphere
float atmosphere(vec3 p, vec3 w, float t)
{
  // density depends on height: d(h) = d0 * exp(-d1*h)
  // d0 is density at water level u_terrain_bounds.x
  // d1 makes it disappear with height h
  // now integrate optical thickness/density:
  // int_s=0..t d(h(s)) ds
  //          = d0 * exp(-d1 * (p.y + s*w.y))
  //          = d0 * exp(-d1 * p.y) + d0 * exp(-d1 * w.y * s)
  // t * d0 * exp(-d1 * p.y) - d0 / (d1 * w.y) * exp(-d1 * w.y * s)
  const float d0 = .0010, d1 = 0.02;
  const float h = p.y - u_terrain_bounds.x;
  // maxima says this is the result:
  return clamp(d0 * (exp(-d1 * h)/(d1*w.y) - exp(-d1*t*w.y -d1 * h)/(d1*w.y)), 0, 1);
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

  uvec2 seed = uvec2((tex_coord.y * u_res.y * u_res.x + u_res.x * tex_coord.x)*100 + u_time *1000, 0);

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
  float shadow = ground_shadow(u_pos + z * w, seed);
  // TODO: exclude explosions / full bright things from shadow computation
  geo_sample.xyz *= 0.5 + shadow * k_sun_col;
  if(env_sample.w > z)
    env_sample = vec4(mix(env_sample.xyz, geo_sample.xyz, geo_sample.w), env_sample.w);

  float sun  = clamp( dot(k_sun_dir,w), 0.0, 1.0 );
  vec3  col  = env_sample.rgb;
  float dist = min(z, env_sample.w);
  // TODO: make sure everything drowns in fog if far away!
  if(u_pos.y < CLOUDS_BOTTOM)
  {
    // boost cloud contribution a bit as compared to ground
    vec4 cld = render_clouds(u_pos, w, dist, seed);
    col = cld.a * col + 1.2 * k_sun_col * cld.rgb;
    vec4 fog = vec4(k_fog_col+shadow*pow(sun, 2.0)*k_sun_col, atmosphere(u_pos, w, dist));
    col = mix(col, fog.rgb, fog.a);
  }
  else
  { // looking from inside/above the cloud layer.
    // this means we first have clouds, then fog, then ground:
    vec4 fog = vec4(k_fog_col+shadow*pow(sun, 2.0)*k_sun_col, atmosphere(u_pos, w, dist));
    col = mix(col, fog.rgb, fog.a);
    vec4 cld = render_clouds(u_pos, w, dist, seed);
    col = cld.a * col + 1.2 * k_sun_col * cld.rgb;
  }

  //------------------------------------------------
  // final
  //------------------------------------------------

  // sun glare (TODO: only if sun is visible at all?)
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
  // find fragment location in last frame!
  dist = min(dist, 2000.0);
  vec4 p_ws = vec4(dist * w, 1);
  // if(cld.a < 0.3)
    p_ws -= vec4(k_cloud_speed/1000.0, 0);
  vec4 p_h = u_mvp_old * p_ws;
  vec2 jitter = .5*p_h.w*vec2(hash1(u_time), hash1(u_time+1337))/u_res.xy;
  p_h += vec4(jitter, 0, 0);
  // compute world space hit point u_pos + dist * w
  // fake motion by undoing wind for clouds
  // take old mvp matrix and apply to the result
  // output screen coordinate here:
  if(dist < z)
    out_motion = p_h.xy / p_h.w;
  else out_motion = geo_mot;
}
