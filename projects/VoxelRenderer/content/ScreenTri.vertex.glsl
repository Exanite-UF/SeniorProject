#version 460 core

out vec2 uv;

void main()
{
    vec4 positionUvs[3];
    positionUvs[0] = vec4(-1, -1, 0, 0);
    positionUvs[1] = vec4(3, -1, 2, 0);
    positionUvs[2] = vec4(-1, 3, 0, 2);

    gl_Position = vec4(positionUvs[gl_VertexID].xy, 0, 1);
    uv = vec2(positionUvs[gl_VertexID].zw);
}
