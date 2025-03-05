#version 460 core

in vec2 uv;

layout(location = 0) out vec4 maskOut;

void main()
{
    maskOut = vec4(1);
}
