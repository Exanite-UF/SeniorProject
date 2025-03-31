#version 460 core

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

uniform ivec3 resolution; //(xSize, ySize, 1)

//[x, y, z]
layout(std430, binding = 0) buffer RayNormal
{
    coherent restrict float rayNormal[];
};

//[x, y, z]
layout(std430, binding = 1) buffer RayPosition
{
    coherent restrict float rayPosition[];
};

//[roughness, motion x, motion y, hue]
layout(std430, binding = 2) buffer RayMisc
{
    coherent restrict float rayMisc[];
};

void main()
{
    ivec3 texelCoord = ivec3(gl_GlobalInvocationID.xyz);

    int index = 3 * (texelCoord.x + texelCoord.y * resolution.x);

    rayNormal[index + 0] = 0;
    rayNormal[index + 1] = 0;
    rayNormal[index + 2] = 0;

    rayPosition[index + 0] = 0;
    rayPosition[index + 1] = 0;
    rayPosition[index + 2] = 0;


    index = 4 * (texelCoord.x + texelCoord.y * resolution.x);

    rayMisc[index + 0] = -1;//By default the primary ray hits a material with negative roughness
    rayMisc[index + 1] = 0;
    rayMisc[index + 2] = 0;
    rayMisc[index + 3] = 0;
}
