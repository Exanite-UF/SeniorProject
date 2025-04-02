#version 460 core

layout(location = 0) in vec3 aPos;
layout(binding = 0) uniform sampler2D positionTexture;
layout(binding = 1) uniform sampler2D normalTexture;

layout(binding = 2) uniform sampler2D sourceTexture;
layout(binding = 3) uniform sampler2D miscTexture;

uniform vec4 inverseCameraRotation;
uniform vec3 cameraPosition;
uniform ivec2 resolution;
uniform float horizontalFovTan; // This equals tan(horizontal fov * 0.5)

out vec2 uv;
out float distance1;
out float isSkyBox;

out vec3 color;
out vec3 normal;
out vec4 misc;
out vec3 posOut;

vec3 qtransform(vec4 q, vec3 v)
{
    return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + q.w * v);
}

float remainder(float a, float m)
{
    float result = mod(a, m);
    if (a < 0)
    {
        result = result - m;
    }
    return result;
}

void main()
{
    posOut = texture(positionTexture, aPos.xy).xyz;
    vec3 pos = posOut;
    vec3 pos2 = mod(pos, 1) - mod(cameraPosition, 1);
    vec3 pos3 = floor(pos) - floor(cameraPosition);
    vec3 pos4 = floor(pos);

    pos -= cameraPosition;
    pos = qtransform(inverseCameraRotation, pos);

    pos2 = qtransform(inverseCameraRotation, pos2);
    pos3 = qtransform(inverseCameraRotation, pos3);

    pos = vec3(-pos.y, pos.z, pos.x);
    distance1 = pos.z;

    pos2 = vec3(-pos2.y, pos2.z, pos2.x);
    pos3 = vec3(-pos3.y, pos3.z, pos3.x);
    pos4 = vec3(-pos4.y, pos4.z, pos4.x);

    vec2 temp = vec2(resolution) / (resolution - 1);

    pos2.xy *= temp + (temp - 1) / max(0, pos.z);
    pos3.xy *= temp + (temp - 1) / max(0, pos.z);

    pos.xy = pos3.xy + pos2.xy;

    normal = texture(normalTexture, aPos.xy).xyz;

    if(length(normal) < 0.5){
        isSkyBox = 1;
    }else{
        isSkyBox = 0;
    }
    
    color = texture(sourceTexture, aPos.xy).xyz;
    misc = texture(miscTexture, aPos.xy);

    pos.y *= resolution.x / float(resolution.y); // Correct for aspect ratio
    pos.z *= horizontalFovTan;

    uv = vec2(aPos.xy);
    gl_Position = vec4(pos.xy, 0.1, pos.z);
}
