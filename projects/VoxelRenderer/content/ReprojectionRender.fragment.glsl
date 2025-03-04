#version 460 core

layout(binding = 1) uniform sampler2D sourceTexture;
layout(binding = 2) uniform sampler2D normalTexture;

in vec2 uv;

out vec4 out_color;

void main()
{
    out_color = vec4(texture(sourceTexture, uv).xyz, 1);
}
