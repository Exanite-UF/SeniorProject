#version 460 core

layout(location = 0) in vec3 aPos;
layout(binding = 0) uniform sampler2D positionTexture;

uniform vec4 inverseCameraRotation;
uniform vec3 cameraPosition;
uniform ivec2 resolution;
uniform float horizontalFovTan; // This equals tan(horizontal fov * 0.5)

out vec2 uv;
out float distance;

vec3 qtransform(vec4 q, vec3 v)
{
    return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + q.w * v);
}

void main()
{
    vec3 pos = texture(positionTexture, aPos.xy).xyz;

    pos -= cameraPosition;
    pos = qtransform(inverseCameraRotation, pos);

    pos = vec3(-pos.y, pos.z, pos.x);
    distance = pos.z;

    pos.y *= resolution.x / float(resolution.y); // Correct for aspect ratio
    pos.z *= horizontalFovTan;

    pos.x *= float(resolution.x) / (resolution.x - 1);
    pos.y *= float(resolution.y) / (resolution.y - 1);

    uv = vec2(aPos.xy);
    gl_Position = vec4(pos.xy, 0.1, pos.z);
}
