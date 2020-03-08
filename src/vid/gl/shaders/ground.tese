#version 450 core
// layout (triangles, equal_spacing, cw) in;
layout (quads, equal_spacing, cw) in;

uniform vec2 u_terrain_bounds;
uniform float u_time;
uniform vec2 u_res;
uniform int u_frame;
uniform float u_lod;
uniform vec3 u_pos_ws;
uniform mat4 u_mvp;
uniform mat4 u_mv;
uniform mat4 u_mvp_old;

out vec3 pos_ws;
out vec4 pos_old;

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
  if(u_lod > 3) return h; // only shading normal, no displacement
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
  return h + fade * mix(s, (.5-dis)/128, clamp(4*matf.g, 0, 1));
#endif
}

void main(void)
{ 
  // vec4 p = // input model coordinates
  //       gl_TessCoord.x*gl_in[0].gl_Position
  //     + gl_TessCoord.y*gl_in[1].gl_Position
  //     + gl_TessCoord.z*gl_in[2].gl_Position;
  // quads:
  vec4 p = mix(
      mix(gl_in[1].gl_Position, gl_in[0].gl_Position, gl_TessCoord.x),
      mix(gl_in[2].gl_Position, gl_in[3].gl_Position, gl_TessCoord.x),
      gl_TessCoord.y);

  p.y = u_terrain_bounds.x - u_pos_ws.y + u_terrain_bounds.y * get_height(vec3(p));
  gl_Position = u_mvp * p;
  pos_ws  = p.xyz;
  pos_old = u_mvp_old * p;
  vec2 jitter = 1.2*gl_Position.w*vec2(hash1(u_time), hash1(u_time+1337))/u_res.xy;
  pos_old += vec4(jitter, 0, 0);
  gl_Position += vec4(jitter, 0, 0);
}
