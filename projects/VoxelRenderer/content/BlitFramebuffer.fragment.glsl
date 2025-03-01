#version 460 core

layout(binding = 0) uniform sampler2D sourceColorTexture;
layout(binding = 1) uniform sampler2D sourceDepthTexture;

in vec2 uv;

out vec4 out_color;

void main()
{
    out_color = texture(sourceColorTexture, uv);
    gl_FragDepth = texture(sourceDepthTexture, uv).x;
}
