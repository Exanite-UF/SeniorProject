#version 460 core

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(std430, binding = 0) buffer PriorAttenuation1
{
    float priorAttenuation1[];
};

layout(std430, binding = 1) buffer AccumulatedLight1
{
    float accumulatedLight1[];
};

layout(std430, binding = 2) buffer PriorAttenuation2
{
    float priorAttenuation2[];
};

layout(std430, binding = 3) buffer AccumulatedLight2
{
    float accumulatedLight2[];
};

layout(std430, binding = 4) buffer FirstHitNormal
{
    float firstHitNormal[];
};

layout(std430, binding = 5) buffer FirstHitPosition
{
    float firstHitPosition[];
};

uniform ivec3 resolution; //(xSize, ySize, raysPerPixel)
uniform bool resetLight;
uniform bool resetAttentuation;

void setAttenuation(ivec3 coord, vec3 value)
{
    int index = 3 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride is 3, axis order is x y z

    priorAttenuation1[0 + index] = value.x;
    priorAttenuation1[1 + index] = value.y;
    priorAttenuation1[2 + index] = value.z;

    priorAttenuation2[0 + index] = value.x;
    priorAttenuation2[1 + index] = value.y;
    priorAttenuation2[2 + index] = value.z;
}

void setLightAccumulation(ivec3 coord, vec3 value)
{
    int index = 3 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride of 3, axis order is x y z
    accumulatedLight1[0 + index] = value.x;
    accumulatedLight1[1 + index] = value.y;
    accumulatedLight1[2 + index] = value.z;

    accumulatedLight2[0 + index] = value.x;
    accumulatedLight2[1 + index] = value.y;
    accumulatedLight2[2 + index] = value.z;
}

void setFirstHitNormal(ivec3 coord, vec3 value)
{
    int index = 3 * (coord.x + resolution.x * (coord.y)); // Stride of 3, axis order is x y z
    firstHitNormal[0 + index] = value.x;
    firstHitNormal[1 + index] = value.y;
    firstHitNormal[2 + index] = value.z;
}

void setFirstHitPosition(ivec3 coord, vec3 value)
{
    int index = 3 * (coord.x + resolution.x * (coord.y)); // Stride of 3, axis order is x y z
    firstHitPosition[0 + index] = value.x;
    firstHitPosition[1 + index] = value.y;
    firstHitPosition[2 + index] = value.z;
}

// layout(rgba32f, binding = 0) uniform writeonly image3D hitPosition;
// layout(rgba32f, binding = 1) uniform writeonly image3D hitNormal;
// layout(r16ui, binding = 2) uniform writeonly uimage3D hitMaterial;

void main()
{
    ivec3 texelCoord = ivec3(gl_GlobalInvocationID.xyz);

    if (resetAttentuation)
    {
        setAttenuation(texelCoord, vec3(1));
    }

    if (resetLight)
    {
        setLightAccumulation(texelCoord, vec3(0));
    }

    if (texelCoord.z == 0)
    {
        setFirstHitNormal(texelCoord, vec3(0));
        setFirstHitPosition(texelCoord, vec3(0));
    }

    // imageStore(hitPosition, texelCoord, vec4(0));
    // imageStore(hitNormal, texelCoord, vec4(vec3(0), 1.0 / 0.0));
    // imageStore(hitMaterial, texelCoord, uvec4(0));
}
