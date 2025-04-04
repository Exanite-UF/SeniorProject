#version 460 core

layout(binding = 0) uniform sampler2D inputTexture;

in vec2 uv;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec3 posBuffer; // Global space
layout(location = 2) out vec3 normalBuffer; // Camera space
layout(location = 3) out vec3 materialBuffer;

void main()
{
    fragColor = vec4(texture(inputTexture, uv).xyz, 1);
    posBuffer = vec3(0);
    normalBuffer = vec3(0);
    materialBuffer = vec3(0);
}
