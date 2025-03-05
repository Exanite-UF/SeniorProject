#version 460 core

layout(binding = 0) uniform sampler2D sourceColorTexture;

in vec2 uv;

out vec4 out_color;

void main()
{
    out_color = texture(sourceColorTexture, uv);
}
