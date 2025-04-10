#version 460 core
#extension GL_EXT_shader_explicit_arithmetic_types_float16 : enable
#extension GL_NV_gpu_shader5 : enable

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

uniform ivec3 resolution; //(xSize, ySize, raysPerPixel)
uniform float maxDepth;

layout(std430, binding = 0) buffer PriorAttenuation1
{
    float16_t priorAttenuation1[];
};

layout(std430, binding = 1) buffer AccumulatedLight1
{
    float16_t accumulatedLight1[];
};

layout(std430, binding = 2) buffer PriorAttenuation2
{
    float16_t priorAttenuation2[];
};

layout(std430, binding = 3) buffer AccumulatedLight2
{
    float16_t accumulatedLight2[];
};

layout(std430, binding = 4) buffer RayMisc
{
    float rayMisc[];
};

void setRayDepth(ivec3 coord, float value)
{
    int index = 1 * (coord.x + resolution.x * coord.y); // axis order is x y
    rayMisc[index + 0] = value;
}

void setAttenuation(ivec3 coord, vec3 value)
{
    int index = 3 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride is 3, axis order is x y z

    priorAttenuation1[0 + index] = float16_t(value.x);
    priorAttenuation1[1 + index] = float16_t(value.y);
    priorAttenuation1[2 + index] = float16_t(value.z);

    priorAttenuation2[0 + index] = float16_t(value.x);
    priorAttenuation2[1 + index] = float16_t(value.y);
    priorAttenuation2[2 + index] = float16_t(value.z);
}

vec3 getAttenuation(ivec3 coord)
{
    int index = 3 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride is 3, axis order is x y z
    return vec3(priorAttenuation1[index + 0], priorAttenuation1[index + 1], priorAttenuation1[index + 2]);
}

void setLightAccumulation(ivec3 coord, vec3 value)
{
    int index = 3 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride of 3, axis order is x y z

    accumulatedLight1[0 + index] = float16_t(value.x);
    accumulatedLight1[1 + index] = float16_t(value.y);
    accumulatedLight1[2 + index] = float16_t(value.z);

    accumulatedLight2[0 + index] = float16_t(value.x);
    accumulatedLight2[1 + index] = float16_t(value.y);
    accumulatedLight2[2 + index] = float16_t(value.z);
}

void main()
{
    ivec3 texelCoord = ivec3(gl_GlobalInvocationID.xyz);

    setAttenuation(texelCoord, vec3(1));
    setLightAccumulation(texelCoord, vec3(0));
    setRayDepth(texelCoord, maxDepth);
}
