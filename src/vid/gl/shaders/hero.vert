#version 440

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 uvs;  // super wacky integer in 3rd channel..

layout(std430, binding = 0) buffer geo_instance
{
  mat4 mat[][2]; // mv mvp per instance
};

uniform uint u_instance_offset;

out vec3 shading_normal;
out vec3 position_cs;
out vec4 old_pos4;
out vec3 tex_uv;

void main()
{
  // transform model to view:
  mat4 mv  = transpose(mat[u_instance_offset+gl_InstanceID][0]);
  mat4 mvp = transpose(mat[u_instance_offset+gl_InstanceID][1]);
  gl_Position = mvp * vec4(position, 1);
  old_pos4 = mvp * vec4(position, 1); // XXX TODO: bring back old positions for taa
  tex_uv = uvs;
  position_cs    = vec3(mv * vec4(position, 1));
  shading_normal = vec3(mv * vec4(normal, 0));
}
