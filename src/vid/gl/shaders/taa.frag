#version 440
in vec2 tex_coord;
layout(binding = 0) uniform sampler2D old_colour;
layout(binding = 1) uniform sampler2D new_colour;
layout(binding = 2) uniform sampler2D new_motion;
uniform vec2 u_res;
uniform float u_lod;
out vec4 frag_color;

// select smallest motion vector in surrounding:
vec2 sample_motion(ivec2 x)
{
  vec2 mm = vec2(0);
  float mml = 100000;
  const int r = 1;
  for(int yy = -r; yy <= r; yy++) {
    for(int xx = -r; xx <= r; xx++) {
      vec2 m = texelFetch(new_motion, x + ivec2(xx, yy), 0).xy;
      float ml = dot(m,m);
      if(ml < mml)
      {
        mml = ml;
        mm = m;
      }
    }
  }
  return mm;
}

// adapted from https://www.shadertoy.com/view/MtVGWz

// note: entirely stolen from https://gist.github.com/TheRealMJP/c83b8c0f46b63f3a88a5986f4fa982b1
//
// Samples a texture with Catmull-Rom filtering, using 9 texture fetches instead of 16.
// See http://vec3.ca/bicubic-filtering-in-fewer-taps/ for more details
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

void main()
{
  ivec2 ipos = ivec2(gl_FragCoord);
  vec4 col = texelFetch(new_colour, ipos, 0);
  vec2 mot = texelFetch(new_motion, ipos, 0).xy;
  // vec2 mot = sample_motion(ipos);
  // vec2 old_tex_coord = tex_coord - mot;
  vec2 old_tex_coord = .5 + .5*mot;

  if(any(lessThan(old_tex_coord,    vec2(0)))
  || any(greaterThan(old_tex_coord, vec2(1.0))))
  {
    frag_color = col;
    return;
  }

  vec3 mom1 = vec3(0.0f);
  vec3 mom2 = vec3(0.0f);
  const int r = 1;
  for(int yy = -r; yy <= r; yy++) {
    for(int xx = -r; xx <= r; xx++) {
      vec3 c = texelFetch(new_colour, ipos + ivec2(xx, yy), 0).rgb;
      mom1 += c;
      mom2 += c * c;
    }
  }
  mom1 /= (2.0 * r + 1) * (2.0 * r + 1);
  mom2 /= (2.0 * r + 1) * (2.0 * r + 1);

  vec3 sigma = sqrt(max(vec3(0), mom2 - mom1 * mom1));

  vec4 color_prev = sample_catmull_rom(old_colour, old_tex_coord);

#if 0
  if(u_lod > 0 && col.w > 0)
  {
    // geo overlay will be marked with 0 in alpha channel here
    float add = 0;
    if(color_prev.w > 0)
      add = 4*(u_lod - 1.0) * length(old_tex_coord*2.0-1.0);
    // only a good idea for bg. raster geo completely destroyed with this:
    const float thresh = 1.0 + add;
    color_prev.rgb = clamp(color_prev.rgb, mom1 - thresh * sigma, mom1 + thresh * sigma);
    // mix in a share of the new image
    frag_color = vec4(mix(color_prev.rgb, col.rgb, vec3(0.1)), 1);
  }
  else
#endif
  {
    const float thresh = 1.0;
    color_prev.rgb = clamp(color_prev.rgb, max(vec3(0),mom1 - thresh * sigma), mom1 + thresh * sigma);
    // color_prev.rgb = clamp(color_prev.rgb, vec3(0), vec3(1));

    // mix in a share of the new image
    frag_color = vec4(mix(color_prev.rgb, col.rgb, vec3(0.1)), 0);
  }
  // DEBUG: show old screen position
  // frag_color = vec4(old_tex_coord, 0, 0);
}
