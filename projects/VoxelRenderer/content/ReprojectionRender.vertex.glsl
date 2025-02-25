#version 460 core

layout (location = 0) in vec3 aPos;
layout(binding = 1) uniform sampler2D positionTexture;

uniform vec4 inverseCameraRotation;
uniform vec3 cameraPosition;
uniform ivec2 resolution;

out vec2 uv;

vec3 qtransform(vec4 q, vec3 v)
{
    return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + q.w * v);
}

void main()
{
    vec4 positionUvs[3];
    positionUvs[0] = vec4(-1, -1, 0, 0);
    positionUvs[1] = vec4(3, -1, 2, 0);
    positionUvs[2] = vec4(-1, 3, 0, 2);

    vec3 pos = texture(positionTexture, aPos.xy).xyz;

    pos -= cameraPosition;
    pos = qtransform(inverseCameraRotation, pos);

    pos = vec3(-pos.y, pos.z, pos.x);
    pos.y *= resolution.x / float(resolution.y);//Correct for aspect ratio

    uv = vec2(aPos.xy);
    gl_Position = vec4(pos, pos.z);
    //gl_Position = vec4((uv - 0.5) * 2, 0, 1);
}
