#version 450 core

layout (vertices = 4) out;

uniform vec2  u_res;
uniform mat4  u_mvp;
uniform float u_lod;

bool offscreen(vec4 vertex)
{
  if(vertex.z < -0.5) return true;
  return any(lessThan(vertex.xy, vec2(-1.7))) ||
    any(greaterThan(vertex.xy, vec2(1.7)));
}

vec4 project(vec4 vertex)
{
  vec4 result = u_mvp * vertex;
  result /= result.w;
  return result;
}

vec2 screen_space(vec4 vertex)
{
  return (clamp(vertex.xy, -1.3, 1.3)+1) * (u_res*0.5);
}

float dist_lod(vec4 v0, vec4 v1)
{
  return 8.0/(u_lod*.5*(v0.z+v1.z));
}

float level(vec2 v0, vec2 v1)
{
  // TODO: also clamp to the world space distance, no need to go smaller than texture size
  // this is with the 16x16 characters:
  // const float k_terrain_pixel_scale = 1.0f/(16.0*1024.0/(3.0*0.3048*2048.0));
  // const float k_terrain_pixel_scale = 16.0f/(16.0*1024.0/(3.0*0.3048*2048.0));
  return clamp(distance(v0, v1)/u_lod, 1, 20);
}

void main()
{
  if(gl_InvocationID == 0)
  {
    vec4 v0 = project(gl_in[0].gl_Position);
    vec4 v1 = project(gl_in[1].gl_Position);
    vec4 v2 = project(gl_in[2].gl_Position);
    vec4 v3 = project(gl_in[3].gl_Position);

    if(all(bvec4(offscreen(v0), offscreen(v1), offscreen(v2), offscreen(v3))))
    // if(all(bvec3(offscreen(v0), offscreen(v1), offscreen(v2))))
    {
      gl_TessLevelInner[0] = 0;
      gl_TessLevelInner[1] = 0;
      gl_TessLevelOuter[0] = 0;
      gl_TessLevelOuter[1] = 0;
      gl_TessLevelOuter[2] = 0;
      gl_TessLevelOuter[3] = 0;
    }
    else
    {
#if 1
      vec2 ss0 = screen_space(v0);
      vec2 ss1 = screen_space(v1);
      vec2 ss2 = screen_space(v2);
      vec2 ss3 = screen_space(v3);

      // float e0 = level(ss1, ss2);
      // float e1 = level(ss0, ss2);
      // float e2 = level(ss1, ss0);
      float e0 = level(ss1, ss2);
      float e1 = level(ss0, ss1);
      float e2 = level(ss3, ss0);
      float e3 = level(ss2, ss3);
#else
      float e0 = dist_lod(v1, v2);
      float e1 = dist_lod(v0, v1);
      float e2 = dist_lod(v3, v0);
      float e3 = dist_lod(v2, v3);
#endif

      gl_TessLevelInner[0] = mix(e1, e2, 0.5);
      gl_TessLevelInner[1] = mix(e0, e3, 0.5);
      gl_TessLevelOuter[0] = e0;
      gl_TessLevelOuter[1] = e1;
      gl_TessLevelOuter[2] = e2;
      gl_TessLevelOuter[3] = e3;
    }
  }
  gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}
