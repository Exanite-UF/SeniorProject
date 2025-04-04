#version 460 core

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

uniform ivec3 resolution; //(xSize, ySize, 1)
uniform float maxDepth;

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

layout(std430, binding = 5) buffer RayStartPosition
{
    float rayStartPosition[];
};

vec3 getRayPosition(ivec3 coord)
{
    int index = 3 * (coord.x + resolution.x * coord.y); // Stride of 3, axis order is x y
    return vec3(rayStartPosition[0 + index], rayStartPosition[1 + index], rayStartPosition[2 + index]);
}

layout(std430, binding = 6) buffer RayDirection
{
    float rayDirection[];
};

vec3 getRayDirection(ivec3 coord)
{
    int index = 3 * (coord.x + resolution.x * coord.y); // Stride of 3, axis order is x y

    return vec3(rayDirection[0 + index], rayDirection[1 + index], rayDirection[2 + index]);
}

void main()
{
    ivec3 texelCoord = ivec3(gl_GlobalInvocationID.xyz);

    int index = 3 * (texelCoord.x + texelCoord.y * resolution.x);

    rayNormal[index + 0] = 0;
    rayNormal[index + 1] = 0;
    rayNormal[index + 2] = 0;

    vec3 rayPos = getRayDirection(texelCoord) * maxDepth + getRayPosition(texelCoord);
    rayPosition[index + 0] = rayPos.x;
    rayPosition[index + 1] = rayPos.y;
    rayPosition[index + 2] = rayPos.z;

    index = 4 * (texelCoord.x + texelCoord.y * resolution.x);

    rayMisc[index + 0] = -1; // By default the primary ray hits a material with negative roughness
    // rayMisc[index + 1] = 0; // these need to persist
    // rayMisc[index + 2] = 0; // these need to persist
    rayMisc[index + 3] = 0;
}
