#version 450

in vec2 tex_uv;
in vec3 colour;
out vec4 frag_color;
layout(binding = 0) uniform sampler2D font;

void main()
{
  vec4 col = texture(font, tex_uv.xy);
  frag_color = vec4(colour, col.w);
}
