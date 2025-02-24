#version 460 core

layout(binding = 0) uniform sampler2D sourceColorTexture;
layout(binding = 1) uniform sampler2D sourceDepthTexture;

in vec2 uv;

out vec4 out_color;

void main()
{
    out_color = texture(sourceColorTexture, uv);
    //out_color = vec4(vec3(texture(sourceDepthTexture, uv).x), 1);
    gl_FragDepth = texture(sourceDepthTexture, uv).x;
}
