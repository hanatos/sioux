#version 440
#define M_PI 3.1415926535897932384626433832795
in vec2 tex_coord;
uniform vec2 u_res;
uniform float u_lod;
uniform float u_hfov;
uniform float u_vfov;
uniform float u_time;
uniform vec2 u_terrain_bounds;
uniform vec3 u_pos;
uniform vec3 u_heli_pos;
uniform vec4 u_orient;
uniform vec3 u_old_pos;
uniform vec4 u_old_orient;
layout(binding = 0) uniform sampler2D old_render;
layout(binding = 1) uniform sampler2D terrain_col;
layout(binding = 2) uniform sampler2D terrain_dis;
layout(binding = 3) uniform sampler2D terrain_det;
layout(binding = 4) uniform sampler2D terrain_ccol;
layout(binding = 5) uniform sampler2D terrain_cdis;
layout(binding = 6) uniform sampler2D terrain_cmat;
layout(binding = 7) uniform sampler2D terrain_acc;
layout(binding = 8) uniform sampler2D terrain_cacc;
layout(binding = 9) uniform sampler2D raster_geo;
layout(binding = 10) uniform sampler2D geo_motion;
layout(location = 0) out vec3 out_colour;
layout(location = 1) out vec2 out_motion;

// to the sides, how large is the texture tile
// convert from something to yards and then to meters:
const float k_terrain_scale = 1.0/(3.0*0.3048*2048.0);

float hash1( vec2 p )
{
  p  = 50.0*fract( p*0.3183099 );
  return fract( p.x*p.y*(p.x+p.y) );
}

float hash1( float n )
{
  return fract( n*17.0*fract( n*0.3183099 ) );
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

// bilinear patch intersectors from
// https://www.shadertoy.com/view/ltKBzG by speps
float patch_intersect(
    in vec3 ro, // ray origin
    in vec3 rd, // ray direction
    in vec4 ps, // size min in xy and max in zw
    in vec4 ph) // four heights of the corners
{
  vec3 va = vec3(0.0, 0.0, ph.x + ph.w - ph.y - ph.z);
  vec3 vb = vec3(0.0, ps.w - ps.y, ph.z - ph.x);
  vec3 vc = vec3(ps.z - ps.x, 0.0, ph.y - ph.x);
  vec3 vd = vec3(ps.xy, ph.x);

  float tmp = 1.0 / (vb.y * vc.x);
  float a = 0.0;
  float b = 0.0;
  float c = 0.0;
  float d = va.z * tmp;
  float e = 0.0;
  float f = 0.0;
  float g = (vc.z * vb.y - vd.y * va.z) * tmp;
  float h = (vb.z * vc.x - va.z * vd.x) * tmp;
  float i = -1.0;
  float j = (vd.x * vd.y * va.z + vd.z * vb.y * vc.x) * tmp
    - (vd.y * vb.z * vc.x + vd.x * vc.z * vb.y) * tmp;

  float p = dot(vec3(a, b, c), rd.xzy * rd.xzy)
    + dot(vec3(d, e, f), rd.xzy * rd.zyx);
  float q = dot(vec3(2.0, 2.0, 2.0) * ro.xzy * rd.xyz, vec3(a, b, c))
    + dot(ro.xzz * rd.zxy, vec3(d, d, e))
    + dot(ro.yyx * rd.zxy, vec3(e, f, f))
    + dot(vec3(g, h, i), rd.xzy);
  float r = dot(vec3(a, b, c), ro.xzy * ro.xzy)
    + dot(vec3(d, e, f), ro.xzy * ro.zyx)
    + dot(vec3(g, h, i), ro.xzy) + j;
  if (abs(p) < 0.000001) {
    return -r / q;
  } else {
    float sq = q * q - 4.0 * p * r;
    if (sq < 0.0) {
      return 0.0;
    } else {
      float s = sqrt(sq);
      float t0 = (-q + s) / (2.0 * p);
      float t1 = (-q - s) / (2.0 * p);
      return min(t0 < 0.0 ? t1 : t0, t1 < 0.0 ? t0 : t1);
    }
  }
}

vec3 patch_normal(in vec4 ps, in vec4 ph, in vec3 pos)
{
  vec3 va = vec3(0.0, 0.0, ph.x + ph.w - ph.y - ph.z);
  vec3 vb = vec3(0.0, ps.w - ps.y, ph.z - ph.x);
  vec3 vc = vec3(ps.z - ps.x, 0.0, ph.y - ph.x);
  vec3 vd = vec3(ps.xy, ph.x);

  float tmp = 1.0 / (vb.y * vc.x);
  float a = 0.0;
  float b = 0.0;
  float c = 0.0;
  float d = va.z * tmp;
  float e = 0.0;
  float f = 0.0;
  float g = (vc.z * vb.y - vd.y * va.z) * tmp;
  float h = (vb.z * vc.x - va.z * vd.x) * tmp;
  float i = -1.0;
  float j = (vd.x * vd.y * va.z + vd.z * vb.y * vc.x) * tmp
    - (vd.y * vb.z * vc.x + vd.x * vc.z * vb.y) * tmp;

  vec3 grad = vec3(2.0) * pos.xzy * vec3(a, b, c)
    + pos.zxz * vec3(d, d, e)
    + pos.yyx * vec3(f, e, f)
    + vec3(g, h, i);
  return -normalize(grad);
}

#if 1
// float moeller trumbore, epsilon free
float tri_intersect(
    vec3 v0, vec3 v1, vec3 v2,
    vec3 ro, vec3 rd,
    out vec2 uv)
{
  vec3 edge1, edge2, tvec, pvec, qvec;
  float det, inv_det;

  edge1 = v1 - v0;
  edge2 = v2 - v0;

  pvec = cross(rd, edge2);
  det = dot(edge1, pvec);
  inv_det = 1.0 / det;

  tvec = ro - v0;

  float v = dot(tvec, pvec) * inv_det;
  if (v < 0.0 || v > 1.0) return -1;

  qvec = cross(tvec, edge1);

  float u = dot(rd, qvec) * inv_det;
  if (u < 0.0 || u + v > 1.0) return -1;

  float dist = dot(edge2, qvec) * inv_det;
  if(dist > 0)//if(dist > tmin && dist <= tmax)
  {
    uv = vec2(u, v);
    return dist;
  }
  return -1;
}
float tri2_intersect(
    vec3 ro, vec3 rd,
    vec4 ps, vec4 ph)
{
  vec2 uv;
  vec3 v0 = vec3(ps.x, ph.x, ps.y);
  vec3 v1 = vec3(ps.z, ph.y, ps.y);
  vec3 v2 = vec3(ps.x, ph.z, ps.w);
  vec3 v3 = vec3(ps.z, ph.w, ps.w);
  float t0 = tri_intersect(v0, v1, v2, ro, rd, uv);
  float t1 = tri_intersect(v1, v2, v3, ro, rd, uv);
  return min(t0 > 0 ? t0 : t1, t1 > 0 ? t1 : t0);
}
#endif

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
// terrain
//------------------------------------------------------------------------------------------

float
raytrace_terrain(
    vec3 ro,  // initial ray origin
    vec3 rd,  // initial ray direction
    float tmin,
    float tmax)
{
  // ==============
  // i3d 08
  // Maximum Mipmaps for Fast, Accurate, and Scalable Dynamic Height Field Rendering
  // Art Tevs, Ivo Ihrke and Hans-Peter Seidel
  // (one of the most badly written papers i ever implemented)
  float t = tmin;
  // FIXME for whatever reason this is still buggy and doesn't go well above 8
  const int max_level = 8; // 10; // XXX get from somewhere
  int level = max_level;
  int lod = 0;
  // convert to texture space:
  ivec2 ts = textureSize(terrain_dis, 0);
  ro.xz *= -k_terrain_scale * ts;
  rd.xz *= -k_terrain_scale * ts;
  // FIXME this doesn't always work
  // TODO: probably have to do multi-level bresenham
  vec3 eps = sign(rd)*vec3(1e-3, 0, 1e-3); // just enough to be on the other side of the texel
  vec3 pos = ro + tmin * rd;
  for(int i=0;i<150;i++)
  {
    // vec3 pos = ro + t * rd + eps;
    if(rd.y >= 0 && pos.y > u_terrain_bounds.x+u_terrain_bounds.y) return -1;
    ivec2 tc = ivec2(floor(pos.xz)) % ts;
    float height = u_terrain_bounds.x + u_terrain_bounds.y*
      texelFetch(terrain_acc, tc>>level, level).r;

    // TODO:
    // surprisingly none of this makes much of a perf difference
    // if(switch_lod(pos, height, lod)) lod++;
    // if(t > 100) lod = 1;
    // if(t > 200) lod = 2;
    // if(t > 400) lod = 3;
#if 0 // this, however, has a speed impact, together with limiting max i
    if(i > 15) lod = 1;
    if(i > 20) lod = 3;
#endif

    {
      // intersect boundary planes (pos, dir, level);
      // these are 5 planes:
      // TODO: most tests are done twice and more often, this sucks:
      // float h = pow(2, level)/ts.x; // <= faster
      // float h = 0.5/textureSize(terrain_dis, level).x;
      // vec2 tc = (floor(ts.xy * pos.xz))/ts.xy;
      ivec2 tc0 = (ivec2(floor(pos.xz)) >> level) << level;
      // ivec2 tc0 = ivec2(floor(pos.xz/(1<<level)))*(1<<level);
      ivec2 tc1 = tc0 + (1<<level);

      vec2 t0 = (tc0 - ro.xz)/rd.xz;
      vec2 t1 = (tc1 - ro.xz)/rd.xz;

      // TODO: this logic is stupid. guess we get away with simpler >/< height tests
      float t_exit  = max(t, min(max(t0.x, t1.x), max(t0.y, t1.y)));
      float t_entry = max(t, max(min(t0.x, t1.x), min(t0.y, t1.y)));

      float h_exit  = ro.y + rd.y * t_exit;
      float h_entry = ro.y + rd.y * t_entry;
      
      if(h_exit > height && h_entry > height)
      { // miss bounding box
        t = t_exit;
        pos = ro + t * rd + eps;
        level = min(max_level, level+1);
        // level = max_level;
      }
      else if(level == lod)// && pos.y <= height)
      { // intersect patch
        if(lod > 0)
          return t;
        vec4 ps = vec4(tc0, tc1);
        vec4 ph = vec4(
            u_terrain_bounds.x + u_terrain_bounds.y*vec4(
              texelFetch(terrain_dis, (ivec2(tc0.x, tc0.y)%ts)>>0, 0).r,
              texelFetch(terrain_dis, (ivec2(tc1.x, tc0.y)%ts)>>0, 0).r,
              texelFetch(terrain_dis, (ivec2(tc0.x, tc1.y)%ts)>>0, 0).r,
              texelFetch(terrain_dis, (ivec2(tc1.x, tc1.y)%ts)>>0, 0).r));
        // FIXME: bilinear patches are still
        //        a desaster, even tris have holes
        // float dist = patch_intersect(ro, rd, ps, ph);
        float dist = tri2_intersect(ro, rd, ps, ph);
        // pos = ro + dist * rd;
        // float ep = 1e-2f;
        if(dist > 0)// &&
           //pos.x - ep <= ps.z && pos.x + ep >= ps.x &&
           //pos.z - ep <= ps.w && pos.z + ep >= ps.y)
          return dist;
        // proceed as if we had missed the voxel
        t = t_exit;
        level = min(max_level, level+1);
        pos = ro + t * rd + eps;
      }
      else if(h_entry > height && h_exit < height)
      { // intersect bounding box from above
        t = (height - ro.y)/rd.y;
        pos = ro + t * rd;
        level = max(level-1, lod);
      }
      else
      { // intersect in all other ways
        t = t_entry;
        vec3 npos = ro + t * rd;
        // "care has to be taken when determing the sampling position."
        // yes, exactly! i'm sure that's not what they meant:
        // measureable speed impact and slightly more precise, not really though:
        // if(rd.x > 0 && fract(npos.x) > 0.99) npos.x += eps.x;
        // if(rd.x < 0 && fract(npos.x) < 0.01) npos.x += eps.x;
        // if(rd.y > 0 && fract(npos.y) > 0.99) npos.y += eps.y;
        // if(rd.y < 0 && fract(npos.y) < 0.01) npos.y += eps.y;
        // if((fract(npos.x) < 0.01 || fract(npos.x) > 0.99)) npos.x += eps.x;
        // if((fract(npos.z) < 0.01 || fract(npos.z) > 0.99)) npos.z += eps.z;
        pos = npos + eps;
        level = max(level-1, lod);
      }
    }
  }
  return -1;
}

float // return distance
raymarch_terrain(
    in vec3 ro,  // initial ray origin
    in vec3 rd,  // initial ray direction
    float tmin,
    float tmax,
    float rand,   // random ray marching offset
    out vec3 pos, // output position
    out vec4 L_s) // in-scattered light
{
#if 1 // simple plane intersection
  {
  float t = -(ro.y - u_terrain_bounds.x + 100)/rd.y;
  if(t < tmin || t > tmax) t = -1;
  return t;
  }
#endif

  float t = 0.0, old_t = 0.0;
  float dt = 0.8;
  float hgt = 10000;
  int lod = 0;
  for(int i=0;i<154;i++)
  {
    float old_hgt = hgt;
    pos = ro + t*rd;
    if(rd.y >= 0 && pos.y > u_terrain_bounds.x+u_terrain_bounds.y) return -1;
    vec2 tc = k_terrain_scale*vec2(-pos.x, -pos.z);
    float dist = u_terrain_bounds.x + u_terrain_bounds.y*
      textureLod(terrain_dis, tc, lod).r;
    // float mdist = u_terrain_bounds.x + u_terrain_bounds.y*
    //   textureLod(terrain_dis, tc, lod+1).r;
    // if(mdist - dist == 0) return -1;
    if(false && t < 100)
    {
      float mat = 256*texture(terrain_det, tc).r;
      float dis = (texelFetch(
          terrain_cdis, ivec2(mod(16*1024*tc, vec2(16)))+ivec2(0, 16*mat), 0).r-.5);
      dist += dis*5;// * u_terrain_bounds.y;// / 128.0;
    }
    hgt = pos.y - dist;
    if(t < 150 && hgt < 0 && dt > 0.1)
    {
      t = old_t;
      hgt = old_hgt;
      dt *= 0.5f;
    }
    old_t = t;
    // else
    if(hgt < 0.0)
    {
      // interpolate:
      t = t + old_hgt / (old_hgt - hgt) * (t - old_t);
      return t;
    }
    else if(t < 150)
    {
      t += max(0.5, hgt*dt);
    }
    else if(t < 300)
    {
      dt = 0.8;
      t += max(2,hgt*dt);
    }
    else if(t < 600)
    {
      dt = 0.9;
      t += max(4, hgt*dt);
      lod = 1;
    }
    else
    {
      dt = 0.9;
      t += max(50,hgt*dt);
      lod = 2;
    }
  }
  //return -1;
  return t; // this is cheating a fair bit but mostly looks okay
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

vec4 render_terrain(
    in vec3 ro,
    in vec3 rd,
    float tmin,
    float tmax,
    float rand,
    out vec4 L_s) // collect in-scattered light (godrays)
{
  vec4 res = vec4(0.0);

  // collect shading data:
  float water_fresnel = 1.0;
  vec3 water_col = vec3(0.0, 0.0, 0.0);
  // trees:
  float tree_hei, tree_mid, tree_dis;
  vec3 underwater = vec3(0.02, 0.03, 0.06);
  vec3 shallowwater = vec3(0.8,0.9,0.6);//vec3(0.08, 0.20, 0.20);
  vec3 pos, col;


  float t = -1;
  if(true)//tex_coord.x > 0.5)
    t = raymarch_terrain(
        ro, rd, tmin, tmax, rand, pos, L_s);
  else 
    t = raytrace_terrain(
        ro, rd, tmin, tmax);

  pos = ro + t*rd;

  if(t > 0.0)
  { // ground
    vec2 tc = k_terrain_scale*vec2(-pos.x, -pos.z);
    col = vec3(1);// XXX texture(terrain_col, tc).rgb;
    vec3 nor = vec3(0, 1, 0);

    if(t < 150)
    {
      ivec3 mat = ivec3(mod(
            // int(u_time)
            // 128
            255-64-ivec3(256*texture(terrain_det, tc).rgb), 256));
      vec4 ccol = texelFetch(
          terrain_ccol, ivec2(mod(16*1024*tc, 16))+ivec2(0, 16*mat.b), 0);
      vec4 cmat = texelFetch(
          terrain_cmat, ivec2(mod(16*1024*tc, 16))+ivec2(0, 16*mat.x), 0);

      // col *= 2*cmat.r; // or additive? or side displacement?
      col = (1.0-cmat.w) * col + cmat.w * 
        //(col + cmat.r-0.5);
        col * 2 * cmat.r;
      col = (1.0-ccol.w) * col + ccol.w * ccol.rgb;
      // col = mat/256.0;
    }

    vec3  ref = reflect(rd,nor);
    float bac = clamp( dot(normalize(vec3(-k_sun_dir.x,0.0,-k_sun_dir.z)),nor), 0.0, 1.0 )*clamp( (pos.y+100.0)/100.0, 0.0,1.0);
    float dom = clamp( 0.5 + 0.5*nor.y, 0.0, 1.0 );
    vec3  lin  = 1.0*0.2*mix(0.1*vec3(0.1,0.2,0.0),vec3(0.7,0.9,1.0),dom);//pow(vec3(occ),vec3(1.5,0.7,0.5));
    lin += 1.0*5.0*vec3(1.0,0.9,0.8); // *dif;        
    lin += 1.0*0.35*vec3(1.0)*bac;

    // col *= lin;

    res = vec4(col, t);
    // XXX DEBUG
    // res = vec4(mod(pos.xz/32.0, 1.0), mod(pos.y/32.0, 1), t);
  }
  else
  { // miss geo, render sky
    res.rgb = renderSky(pos, rd);
    res.w = tmax;
  }

  return res;
}

void main()
{
  // fake anti-aliasing:
  vec2 jitter = vec2(hash1(u_time), hash1(u_time+1337));
  vec2 angle = (tex_coord + jitter * 0.9/u_res) - 0.5;
#if 1 // retinal rendering:
  if(u_lod > 1)
  {
    float tc1 = length(angle);
    float tc2 = (1.-1./u_lod)*tc1*tc1 + 1./u_lod * tc1;
    angle *= tc2/tc1;
  }
#endif
  angle *= vec2(-u_hfov, u_vfov);
  // equi-angular:
  vec3 camray = vec3(sin(angle.x)*cos(angle.y), sin(angle.y), cos(angle.x)*cos(angle.y));
  vec3 w = normalize(quat_transform(camray, u_orient));

#if 0
  {
  // z inside screen; x left
  vec4 ps = vec4(
      u_pos.xz + vec2(-100, 1000+mod(u_time * 200, 5000)),
      u_pos.xz + vec2( 100, 1000+mod(u_time * 400, 5000)));
  // -x-z, x-z, -xz, xz
  vec4 ph = vec4(-200, -200, -200, -200);
  // float dist = patch_intersect(u_pos, camray, ps, ph);
  float dist = tri2_intersect(u_pos, camray, ps, ph);
  vec3 pos = u_pos + dist*camray;
  if(dist > 0
     &&
     pos.x <= ps.z && pos.x >= ps.x &&
     pos.z <= ps.w && pos.z >= ps.y)
    out_colour = vec3(mod(pos.xz/32.0, 1.0), mod(pos.y/32.0, 1));
  else
    out_colour = vec3(0);
  out_motion = vec2(0,0);
  return;
  }
#endif

  vec3 col;
  vec4 terrain_sample;
  vec4 godrays_sample;
  vec4 clouds_sample;
  float clouds_tau = 1.0;

  // TODO: bound by cockpit texture and geo_depth from below!
  // somehow the sky and depth buffers are unhappy with > tmax:
  vec2 bounds = vec2(1.0f, 10000.0f);
  float resT = bounds.y;
  float rand = 0.0;//hash1(tex_coord.x*213 + tex_coord.y * 313 + u_time/10.0)*2;

  //------------------------------------------------
  // backdrop: terrain with trees and water, sky
  //------------------------------------------------
  // terrain_sample = render_terrain(u_pos, w, bounds.x, bounds.y, rand, godrays_sample);
  terrain_sample = vec4(renderSky(u_pos, w), bounds.y);

  //------------------------------------------------
  // raster geo sample
  //------------------------------------------------
  vec4 geo_sample = textureLod(raster_geo, tex_coord, 0);
  // TODO: pass on motion vectors from geo???
  vec3 geo_depth = texture(geo_motion, tex_coord).rgb;
  if(terrain_sample.w > geo_depth.z)
    terrain_sample = vec4(mix(terrain_sample.xyz, geo_sample.xyz, geo_sample.w), terrain_sample.w);//geo_depth);

  col  = terrain_sample.rgb;
  float dist = min(geo_depth.z, terrain_sample.w);
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

  //------------------------------------------------
  // motion vectors
  //------------------------------------------------
  const float n = .10, f = 10000.0, f2 = 9996;
  w = normalize(quat_transform(camray, u_orient));
  vec3 hit = u_pos + dist * w;

  // motion vectors:
  // project to old camera via inverse orientation
  vec3 old_ray = normalize(quat_transform(hit - u_old_pos, quat_inv(u_old_orient)));
  // get tex coord that corresponds best to world space ray:
  vec2 old_angle = vec2(
      atan(old_ray.x, old_ray.z),
      asin(old_ray.y));
  // motion vectors correspond to undistorted texture coords and will be
  // applied after going full res again:
  out_motion = .5*(angle - old_angle)/vec2(u_hfov, u_vfov);
  if(terrain_sample.w > geo_depth.z)
    out_motion = geo_depth.xy;
  // out_colour.xy = 10*sin(out_motion); // oh great, these are broken.
}
