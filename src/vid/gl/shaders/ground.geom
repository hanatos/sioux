#version 450 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

in vec3 te_pos_ws[];
in vec4 te_pos_old[];
out vec3 pos_ws;
out vec4 pos_old;
out vec3 normal;

void main()
{
  normal = normalize(cross(
        te_pos_ws[1]-
        te_pos_ws[0],
        te_pos_ws[2]-
        te_pos_ws[0]));

  pos_ws  = te_pos_ws[0];
  pos_old = te_pos_old[0];
  gl_Position = gl_in[0].gl_Position; EmitVertex();

  pos_ws  = te_pos_ws[1];
  pos_old = te_pos_old[1];
  gl_Position = gl_in[1].gl_Position; EmitVertex();

  pos_ws  = te_pos_ws[2];
  pos_old = te_pos_old[2];
  gl_Position = gl_in[2].gl_Position; EmitVertex();

  EndPrimitive();
}
