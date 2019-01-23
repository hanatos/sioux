#version 440
out vec2 tex_coord;
void
main()
{
  vec2 p;
  if(gl_VertexID == 0)
    p = vec2(-1,  1);
  else if(gl_VertexID == 2)
    p = vec2(-1, -3);
  else
    p = vec2( 3,  1);
  tex_coord = p.xy * 0.5 + 0.5;
  gl_Position = vec4(p, 0.0, 1.0);
}
