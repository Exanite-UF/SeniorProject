#version 460 core

layout(binding = 0) uniform sampler2D positionTexture;
layout(binding = 1) uniform sampler2D normalTexture;
layout(binding = 2) uniform sampler2D sourceTexture;
layout(binding = 3) uniform sampler2D materialTexture;

in vec2 uv;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec3 posBuffer; // Global space
layout(location = 2) out vec3 normalBuffer; // Camera space
layout(location = 3) out vec3 materialBuffer;


void main()
{
    vec3 material = texture(materialTexture, uv).xyz;
    vec3 normal = texture(normalTexture, uv).xyz;


    fragColor = vec4(texture(sourceTexture, uv).xyz, 1);
    posBuffer = texture(positionTexture, uv).xyz;
    normalBuffer = normal;
    materialBuffer = material;
}
