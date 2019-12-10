#version 440

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 uvs;  // super wacky integer in 3rd channel..

uniform mat4 u_mv;
uniform mat4 u_mvp;

out vec3 shading_normal;
out vec3 position_cs;
out vec4 old_pos4;
out vec3 tex_uv;
out vec4 color;

void main()
{
  // transform model to view:
  gl_Position = u_mvp * vec4(position, 1);
  old_pos4 = u_mvp * vec4(position, 1); // XXX TODO: bring back old positions for taa
  tex_uv = uvs;
  position_cs    = vec3(u_mv * vec4(position, 1));
  shading_normal = vec3(u_mv * vec4(normal, 0));
  color = vec4(0.1, 1.0, 1.0, 1.0);
}
