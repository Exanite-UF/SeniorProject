#version 460 core

in vec2 uv;
in float isSkybox;


layout(location = 0) out vec4 maskOut;

void main()
{
    maskOut = vec4(1 - isSkybox);
}
