#version 440

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 uvs;  // super wacky integer in 3rd channel..

layout(std430, binding = 0) buffer geo_instance
{
  mat4 mat[][3]; // mv mvp mvp_old per instance
};

uniform uint u_instance_offset;
uniform float u_time;
uniform vec2 u_res;

out vec3 shading_normal;
out vec3 position_cs;
out vec4 old_pos4;
out vec2 tex_uv;
out flat uvec2 id;

float hash1( float n )
{
  return fract( n*17.0*fract( n*0.3183099 ) );
}

void main()
{
  // transform model to view:
  mat4 mv      = transpose(mat[u_instance_offset+gl_InstanceID][0]);
  mat4 mvp     = transpose(mat[u_instance_offset+gl_InstanceID][1]);
  mat4 mvp_old = transpose(mat[u_instance_offset+gl_InstanceID][2]);
  gl_Position  = mvp * vec4(position, 1);
  old_pos4 = mvp_old * vec4(position, 1);
  vec2 jitter = 1.2*gl_Position.w * vec2(hash1(u_time), hash1(u_time+1337))/u_res;
  gl_Position += vec4(jitter, 0, 0);
  old_pos4    += vec4(jitter, 0, 0);
  tex_uv = uvs.xy;
  id = uvec2(uvs.z, u_instance_offset + gl_InstanceID);
  position_cs    = vec3(mv * vec4(position, 1));
  shading_normal = vec3(mv * vec4(normal, 0));
}
