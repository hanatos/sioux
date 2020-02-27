#version 450
#extension GL_ARB_bindless_texture : require

in vec2 tex_uv;
in vec3 colour;
out vec4 frag_color;
layout(bindless_sampler) uniform sampler2D font;

void main()
{
  vec4 col = texture(font, tex_uv.xy);
  frag_color = vec4(colour, col.w);
}
