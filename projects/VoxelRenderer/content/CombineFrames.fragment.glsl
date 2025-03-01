#version 460 core

layout(binding = 0) uniform sampler2D oldColor;
layout(binding = 1) uniform sampler2D oldPosition;

layout(binding = 2) uniform sampler2D newPosition;
layout(binding = 3) uniform sampler2D newMaterial;

layout(binding = 4) uniform sampler2D frameCount;

layout(binding = 5) uniform sampler2D combineMask;

in vec2 uv;
in float distance;

uniform ivec2 resolution;

layout (location = 0) out vec4 out_color;
layout (location = 1) out vec4 frameCountOut;


void main()
{
    float frameCount = texture(frameCount, uv).x;

    vec2 localUV = gl_FragCoord.xy / resolution.xy;
    vec3 oldPos = texture(oldPosition, uv).xyz;
    vec3 newPos = texture(newPosition, localUV).xyz;
    float dist = length(newPos - oldPos);
    frameCount /= 100 * (dist / distance) + 1;

    frameCount *= texture(combineMask, localUV).x;

    vec3 material = texture(newMaterial, localUV).xyz;

    frameCount *= material.x;

    frameCount = min(frameCount, 10);


    //out_color = vec4(vec3(texture(combineMask, localUV).x), 1);
    out_color = vec4(texture(oldColor, uv).xyz, float(frameCount) / (frameCount + 1));
    frameCountOut = vec4(frameCount + 1, 0, 0, 1);

    //vec2 localUV = gl_FragCoord.xy / resolution.xy;
    //vec3 oldPos = texture(oldPosition, uv).xyz;
    //vec3 newPos = texture(newPosition, localUV).xyz;
    //out_color = vec4(vec3(length(newPos - oldPos)), 1);
}
