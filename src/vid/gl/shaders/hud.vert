#version 450

layout(location = 0) in vec2 position;
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
}
