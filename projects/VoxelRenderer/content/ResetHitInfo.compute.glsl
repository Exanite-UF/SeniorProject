#version 440

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(std430, binding = 0) buffer HitPosition
{
    vec4 hitPosition[];
};
layout(std430, binding = 1) buffer HitNormal
{
    vec4 hitNormal[];
};

layout(std430, binding = 2) buffer HitMaterial
{
    uint hitMaterial[];
};

layout(std430, binding = 3) buffer HitVoxelPosition
{
    float hitVoxelPosition[];
};

layout(std430, binding = 4) buffer PriorAttenuation
{
    float priorAttenuation[];
};

layout(std430, binding = 5) buffer AccumulatedLight
{
    float accumulatedLight[];
};

uniform ivec3 resolution; //(xSize, ySize, raysPerPixel)

void setHitPosition(ivec3 coord, vec4 value)
{
    int index = (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride of 3, axis order is x y z
    hitPosition[index] = value;
}

void setHitNormal(ivec3 coord, vec4 value)
{
    int index = (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride of 3, axis order is x y z
    hitNormal[index] = value;
}

void setHitMaterial(ivec3 coord, uint value)
{
    int index = (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride of 3, axis order is x y z
    hitMaterial[index] = value;
}

void setHitVoxelPosition(ivec3 coord, vec3 value)
{
    int index = 3 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride of 3, axis order is x y z
    hitVoxelPosition[index + 0] = value.x;
    hitVoxelPosition[index + 1] = value.y;
    hitVoxelPosition[index + 2] = value.z;
}

void setAttenuation(ivec3 coord, vec3 value)
{
    int index = 3 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride is 3, axis order is x y z

    priorAttenuation[0 + index] = value.x;
    priorAttenuation[1 + index] = value.y;
    priorAttenuation[2 + index] = value.z;
}

void setLightAccumulation(ivec3 coord, vec3 value)
{
    int index = 3 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride of 3, axis order is x y z
    accumulatedLight[0 + index] = value.x;
    accumulatedLight[1 + index] = value.y;
    accumulatedLight[2 + index] = value.z;
}

// layout(rgba32f, binding = 0) uniform writeonly image3D hitPosition;
// layout(rgba32f, binding = 1) uniform writeonly image3D hitNormal;
// layout(r16ui, binding = 2) uniform writeonly uimage3D hitMaterial;

void main()
{
    ivec3 texelCoord = ivec3(gl_GlobalInvocationID.xyz);

    setHitPosition(texelCoord, vec4(0));
    setHitNormal(texelCoord, vec4(0, 0, 0, 1.0 / 0.0));
    setHitMaterial(texelCoord, 0);
    setHitVoxelPosition(texelCoord, vec3(0));

    setAttenuation(texelCoord, vec3(0));
    setLightAccumulation(texelCoord, vec3(0));
    
    // imageStore(hitPosition, texelCoord, vec4(0));
    // imageStore(hitNormal, texelCoord, vec4(vec3(0), 1.0 / 0.0));
    // imageStore(hitMaterial, texelCoord, uvec4(0));
}
