#version 450 core

uniform vec2 u_terrain_bounds;
uniform float u_time;
uniform vec2 u_res;
uniform int u_frame;
uniform float u_lod;
uniform vec3 u_pos_ws;
uniform vec3 u_pos_coma_ws;

layout(binding = 1) uniform sampler2D terrain_col;
layout(binding = 2) uniform sampler2D terrain_dis;
layout(binding = 3) uniform sampler2D terrain_det;
layout(binding = 4) uniform sampler2D terrain_ccol;
layout(binding = 5) uniform sampler2D terrain_cdis;
layout(binding = 6) uniform sampler2D terrain_cmat;
layout(binding = 7) uniform sampler2D terrain_acc;
layout(binding = 8) uniform sampler2D terrain_cacc;

out vec4 frag_color;
out vec2 out_motion;
in vec4 pos_old;
in vec3 pos_ws;
in vec3 normal;

#if 1

#define RIGID
// Standard 2x2 hash algorithm.
vec2 hash22(vec2 p) {
    
    // Faster, but probaly doesn't disperse things as nicely as other methods.
    float n = sin(dot(p, vec2(113, 1)));
    p = fract(vec2(2097152, 262144)*n)*2. - 1.;
    #ifdef RIGID
    return p;
    #else
    return cos(p*6.283 + iGlobalTime);
    //return abs(fract(p+ iGlobalTime*.25)-.5)*2. - .5; // Snooker.
    //return abs(cos(p*6.283 + iGlobalTime))*.5; // Bounce.
    #endif

}
// Standard 2D rotation formula - Nimitz says it's faster, so that's good enough for me. :)
mat2 rot2(in float a){ float c = cos(a), s = sin(a); return mat2(c, s, -s, c); }
// Gradient noise. Ken Perlin came up with it, or a version of it. Either way, this is
// based on IQ's implementation. It's a pretty simple process: Break space into squares, 
// attach random 2D vectors to each of the square's four vertices, then smoothly 
// interpolate the space between them.
float gradN2D(in vec2 f){
    
    // Used as shorthand to write things like vec3(1, 0, 1) in the short form, e.yxy. 
   const vec2 e = vec2(0, 1);
   
    // Set up the cubic grid.
    // Integer value - unique to each cube, and used as an ID to generate random vectors for the
    // cube vertiies. Note that vertices shared among the cubes have the save random vectors attributed
    // to them.
    vec2 p = floor(f);
    f -= p; // Fractional position within the cube.
    

    // Smoothing - for smooth interpolation. Use the last line see the difference.
    //vec2 w = f*f*f*(f*(f*6.-15.)+10.); // Quintic smoothing. Slower and more squarish, but derivatives are smooth too.
    vec2 w = f*f*(3. - 2.*f); // Cubic smoothing. 
    //vec2 w = f*f*f; w = ( 7. + (w - 7. ) * f ) * w; // Super smooth, but less practical.
    //vec2 w = .5 - .5*cos(f*3.14159); // Cosinusoidal smoothing.
    //vec2 w = f; // No smoothing. Gives a blocky appearance.
    
    // Smoothly interpolating between the four verticies of the square. Due to the shared vertices between
    // grid squares, the result is blending of random values throughout the 2D space. By the way, the "dot" 
    // operation makes most sense visually, but isn't the only metric possible.
    float c = mix(mix(dot(hash22(p + e.xx), f - e.xx), dot(hash22(p + e.yx), f - e.yx), w.x),
                  mix(dot(hash22(p + e.xy), f - e.xy), dot(hash22(p + e.yy), f - e.yy), w.x), w.y);
    
    // Taking the final result, and converting it to the zero to one range.
    return c*.5 + .5; // Range: [0, 1].
}

// Gradient noise fBm.
float fBm(in vec2 p){
    
    return gradN2D(p)*.57 + gradN2D(p*2.)*.28 + gradN2D(p*4.)*.15;
    
}

// Repeat gradient lines. How you produce these depends on the effect you're after. I've used a smoothed
// triangle gradient mixed with a custom smoothed gradient to effect a little sharpness. It was produced
// by trial and error. If you're not sure what it does, just call it individually, and you'll see.
float grad(float x, float offs){
    
    // Repeat triangle wave. The tau factor and ".25" factor aren't necessary, but I wanted its frequency
    // to overlap a sine function.
    x = abs(fract(x/6.283 + offs - .25) - .5)*2.;
    
    float x2 = clamp(x*x*(-1. + 2.*x), 0., 1.); // Customed smoothed, peaky triangle wave.
    //x *= x*x*(x*(x*6. - 15.) + 10.); // Extra smooth.
    x = smoothstep(0., 1., x); // Basic smoothing - Equivalent to: x*x*(3. - 2.*x).
    return mix(x, x2, .15);
    
/*    
    // Repeat sine gradient.
    float s = sin(x + 6.283*offs + 0.);
    return s*.5 + .5;
    // Sine mixed with an absolute sine wave.
    //float sa = sin((x +  6.283*offs)/2.);
    //return mix(s*.5 + .5, 1. - abs(sa), .5);
    
*/
}

// One sand function layer... which is comprised of two mixed, rotated layers of repeat gradients lines.
float sandL(vec2 p){
    
    // Layer one. 
    vec2 q = rot2(3.14159/18.)*p; // Rotate the layer, but not too much.
    q.y += (gradN2D(q*18.) - .5)*.05; // Perturb the lines to make them look wavy.
    float grad1 = grad(q.y*80., 0.); // Repeat gradient lines.
   
    q = rot2(-3.14159/20.)*p; // Rotate the layer back the other way, but not too much.
    q.y += (gradN2D(q*12.) - .5)*.05; // Perturb the lines to make them look wavy.
    float grad2 = grad(q.y*80., .5); // Repeat gradient lines.
      
    
    // Mix the two layers above with an underlying 2D function. The function you choose is up to you,
    // but it's customary to use noise functions. However, in this case, I used a transcendental 
    // combination, because I like the way it looked better.
    // 
    // I feel that rotating the underlying mixing layers adds a little variety. Although, it's not
    // completely necessary.
    q = rot2(3.14159/4.)*p;
    //float c = mix(grad1, grad2, smoothstep(.1, .9, n2D(q*vec2(8))));//smoothstep(.2, .8, n2D(q*8.))
    //float c = mix(grad1, grad2, n2D(q*vec2(6)));//smoothstep(.2, .8, n2D(q*8.))
    //float c = mix(grad1, grad2, dot(sin(q*12. - cos(q.yx*12.)), vec2(.25)) + .5);//smoothstep(.2, .8, n2D(q*8.))
    
    // The mixes above will work, but I wanted to use a subtle screen blend of grad1 and grad2.
    float a2 = dot(sin(q*12. - cos(q.yx*12.)), vec2(.25)) + .5;
    float a1 = 1. - a2;
    
    // Screen blend.
    float c = 1. - (1. - grad1*a1)*(1. - grad2*a2);
    
    // Smooth max\min
    //float c = smax(grad1*a1, grad2*a2, .5);
   
    return c;
    
    
}

// A global value to record the distance from the camera to the hit point. It's used to tone
// down the sand height values that are further away. If you don't do this, really bad
// Moire artifacts will arise. By the way, you should always avoid globals, if you can, but
// I didn't want to pass an extra variable through a bunch of different functions.
// float gT;

float sand(vec2 p){
    
    // Rotating by 45 degrees. I thought it looked a little better this way. Not sure why.
    // I've also zoomed in by a factor of 4.
    p = vec2(p.y - p.x, p.x + p.y)*.7071/4.;
    
    // Sand layer 1.
    float c1 = sandL(p);
    
    // Second layer.
    // Rotate, then increase the frequency -- The latter is optional.
    vec2 q = rot2(3.14159/12.)*p;
    float c2 = sandL(q*1.25);
    
    // Mix the two layers with some underlying gradient noise.
    c1 = mix(c1, c2, smoothstep(.1, .9, gradN2D(p*vec2(4))));
    
/*   
	// Optional screen blending of the layers. I preferred the mix method above.
    float a2 = gradN2D(p*vec2(4));
    float a1 = 1. - a2;
    
    // Screen blend.
    c1 = 1. - (1. - c1*a1)*(1. - c2*a2);
*/    
    
    // Extra grit. Not really necessary.
    //c1 = .7 + fBm(p*128.)*.3;
    
    // A surprizingly simple and efficient hack to get rid of the super annoying Moire pattern 
    // formed in the distance. Simply lessen the value when it's further away. Most people would
    // figure this out pretty quickly, but it took me far too long before it hit me. :)
    return c1;// c1/(1. + gT*gT*.015);
}
#endif

float get_height(vec3 pos)
{
  const float k_terrain_scale = 1.0/(3.0*0.3048*2048.0);
  vec2 uv = -k_terrain_scale * (u_pos_ws.xz + pos.xz);
  float dist = length(pos);
  int lod = 0;
  // lod = clamp(int(dist / 100), 0, 5);
#if 0
  float h = textureLod(terrain_dis, uv, lod).r;
#else
  vec4 off = vec4(
      vec2(0.5)/textureSize(terrain_dis, lod).xy,
     -vec2(0.5)/textureSize(terrain_dis, lod).xy);
  float h0 = textureLod(terrain_dis, uv+off.xy, lod).r;
  float h1 = textureLod(terrain_dis, uv+off.xw, lod).r;
  float h2 = textureLod(terrain_dis, uv+off.zy, lod).r;
  float h3 = textureLod(terrain_dis, uv+off.zw, lod).r;
  float h = 0.25*(h0+h1+h2+h3);
#endif
  if(dist > 1000/u_lod) return h;
  float fade = clamp(1.0-(dist - 700.0/u_lod)/(100.0/u_lod), 0.0, 1.0);
#if 1
  // float mat = 256*texture(terrain_det, uv).r;
  // uvec3 mat = texelFetch(terrain_det, ivec2(1024*uv+.5f), 0).rgb/4;
  // uvec3 mat = textureLod(terrain_det, uv, 0).rgb/4;
  vec3 matf = textureLod(terrain_det, uv, 0).rgb;
  uvec3 mat = uvec3(matf*256.0/4.0);
  uint tile = mat.r + 72;//144;
  if(mat.r == 0) tile = mat.b;

  float s = -0.005+0.01*sand((pos.xz+u_pos_ws.xz)*0.05);
  s += -0.001+.001*sand((pos.xz+u_pos_ws.xz)*0.6);
#if 0
  float dis = texelFetch(
    terrain_cdis, ivec2(mod(16*1024*uv, vec2(16)))+ivec2(0, 16*tile), 0).r;
#else
  float dis = textureLod(
      terrain_cdis, (ivec2(mod(16*1024*uv, vec2(16)))+ivec2(0, 16*tile))/vec2(16.0, 4096.0), 0).r;
#endif
  // if(mat.r != 0) return h;// + (dis-.5)/64.0;
  // if(mat.r == 0) return h + 0.01;
  h += fade * mix(s, (.5-dis)/128, clamp(4*matf.g, 0, 1));
  return u_terrain_bounds.x + u_terrain_bounds.y * h;
#endif
}

#if 0
float get_height(vec3 pos)
{
  const float k_terrain_scale = 1.0/(3.0*0.3048*2048.0);
  vec2 uv = -k_terrain_scale * (u_pos_ws.xz + pos.xz);
  float h = textureLod(terrain_dis, uv, 0).r;
  return u_terrain_bounds.x + u_terrain_bounds.y * h;
}
#endif

vec3 get_normal(vec3 pos)
{
#if 0 // faster but artifacts (11.7ms vs 12.9ms)
  return normalize( cross(dFdx(pos),dFdy(pos)) );
#else
  vec2 off = vec2(0.01, 0);
  float h0 = get_height(pos);
  float h1 = get_height(pos + off.xyy);
  float h2 = get_height(pos + off.yyx);
  return -normalize(cross(vec3(off.x, h1-h0, 0), vec3(0, h2-h0, off.x)));
#endif
}

vec4 get_colour(vec3 pos)
{
  float shadow = length(u_pos_ws.xz + pos.xz - u_pos_coma_ws.xz);
  if(shadow < 20.0)
  { // fake shadow. should probably have dust kicked up here and all
    if(shadow > 19.8) return vec4(0.1, 1, .5, 1); // draw landing helper hud (maybe only if gear down? check distance?)
    float h = clamp(u_pos_coma_ws.y - pos.y - 3, 0., 10000.);
    shadow = clamp(1.0-1.5*exp(-shadow*shadow/50.0 - h * 0.0026), 0., 1.);
  }
  else shadow = 1.0;
  const float k_terrain_scale = 1.0/(3.0*0.3048*2048.0);
  vec2 uv = -k_terrain_scale * (u_pos_ws.xz + pos.xz);
#if 0
  return texture(terrain_col, uv);
#else
  // float mat = 256*texture(terrain_det, uv).r;
  // ivec3 mat = ivec3(256*texelFetch(terrain_det, ivec2(1024*uv+.5f), 0).rgb);
  vec3 base = texture(terrain_col, uv).rgb;

  if(length(pos) > 1000/u_lod) return vec4(base, 1);
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
  return vec4(shadow * mix(base, col.rgb, col.a), 1);
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
  const vec3 k_sun_dir = vec3(-0.624695,0.468521,-0.624695);
  // we just compute the tesselated tri normal to show some
  // "voxel artifacts". really these are to high frequency to be useful as normal.
  const float amb = 0.1;
#if 0 // use geometry shader normal
  float diff = amb + (1.0-amb)*max(0, dot(k_sun_dir, normalize(-normal)));
#else // super expensive normals, could strip away geo shader
  vec3 n = get_normal(pos_ws);
  float diff = amb + (1.0-amb)*max(0, dot(k_sun_dir, normalize(n)));
#endif
  frag_color.rgb = diff*get_colour(pos_ws).rgb;

#if 0
  // TODO: detect grass, rock, water!
  // TODO: try to put in displacement instead (normals?)
  // TODO: maybe instead of other details?
  float s = .1 + sand((pos_ws.xz+u_pos_ws.xz)*0.05);
  s *= .7 + .8*sand((pos_ws.xz+u_pos_ws.xz)*0.6);
  frag_color.rgb *= s;
#endif

  // looks different but also shit. these normals show typical aliasing problems.
  // will need to generate them on the CPU and blur them to hell.
#if 0
  vec3 n = get_normal(pos_ws);
  // frag_color = vec4(.5+.5*n, 1);
  frag_color.rgb *= clamp(dot(n, k_sun_dir)+0.4, 0.2, 1.0);
#endif
  frag_color.a = 1;

  // out_motion = gl_FragCoord.xy - pos_old.xy / pos_old.w;
  out_motion = pos_old.xy / pos_old.w;

  // frag_color = vec4(0.0f, mod(100*tc, 1), 1.0f);
  // frag_color = vec4(0.0f, 0.1*mod(tex_uv3, 1), 1.0f);
}
