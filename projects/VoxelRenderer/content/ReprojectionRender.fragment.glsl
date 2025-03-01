#version 460 core

layout(binding = 0) uniform sampler2D sourceTexture;
layout(binding = 2) uniform sampler2D materialTexture;

in vec2 uv;

out vec4 out_color;

void main()
{
    out_color = texture(sourceTexture, uv);
    //out_color = vec4(vec3(texture(materialTexture, uv).x), 1);
    //out_color = vec4(uv, 0, 1);
    //out_color = vec4(texture(positionTexture, uv).xyz * 0.001, 1);
}
