#version 450

out vec2 tex_uv;
layout(location = 0) in vec2 position;
layout(location = 1) in vec2 uvs;
out vec3 colour;
uniform uint u_flash_beg;
uniform uint u_flash_end;
uniform vec3 u_col;
uniform vec3 u_flash_col;

void main()
{
  if(gl_VertexID >= u_flash_beg && gl_VertexID < u_flash_end)
    colour = u_flash_col;
  else
    colour = u_col;
  gl_Position = vec4(position.x, position.y, 0.0f, 1.0f);
  tex_uv = uvs;
}
