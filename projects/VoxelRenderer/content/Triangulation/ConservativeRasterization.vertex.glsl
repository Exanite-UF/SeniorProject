#version 460 core
layout(location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 worldPosition;

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);
    worldPosition = worldPos.xyz;
    gl_Position = projection * view * worldPos;
}