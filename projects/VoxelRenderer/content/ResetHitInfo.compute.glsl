#version 440

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(std430, binding = 0) buffer HitPosition
{
    float hitPosition[];
};
layout(std430, binding = 1) buffer HitNormal
{
    float hitNormal[];
};

layout(std430, binding = 2) buffer HitMaterial
{
    uint hitMaterial[];
};

layout(std430, binding = 3) buffer HitVoxelPosition
{
    float hitVoxelPosition[];
};

layout(std430, binding = 4) buffer HitMisc
{
    float hitMisc[];
};

uniform ivec3 resolution; //(xSize, ySize, raysPerPixel)

void setHitPosition(ivec3 coord, vec3 value)
{
    int index = 3 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride of 3, axis order is x y z
    hitPosition[index + 0] = value.x;
    hitPosition[index + 1] = value.y;
    hitPosition[index + 2] = value.z;
}

void setHitNormal(ivec3 coord, vec3 value)
{
    int index = 3 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride of 3, axis order is x y z
    hitNormal[index + 0] = value.x;
    hitNormal[index + 1] = value.y;
    hitNormal[index + 2] = value.z;
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

void setHitWasHit(ivec3 coord, bool value)
{
    int index = 2 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // axis order is x y z
    hitMisc[index + 0] = (value) ? 1.0 : 0.0;
}

void setHitDist(ivec3 coord, float value)
{
    int index = 2 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // axis order is x y z
    hitMisc[index + 1] = value;
}

// layout(rgba32f, binding = 0) uniform writeonly image3D hitPosition;
// layout(rgba32f, binding = 1) uniform writeonly image3D hitNormal;
// layout(r16ui, binding = 2) uniform writeonly uimage3D hitMaterial;

void main()
{
    ivec3 texelCoord = ivec3(gl_GlobalInvocationID.xyz);

    setHitPosition(texelCoord, vec3(0));
    setHitNormal(texelCoord, vec3(0));
    setHitMaterial(texelCoord, 0);
    setHitVoxelPosition(texelCoord, vec3(0));
    setHitWasHit(texelCoord, false);
    setHitDist(texelCoord, 1.0 / 0.0);

    // imageStore(hitPosition, texelCoord, vec4(0));
    // imageStore(hitNormal, texelCoord, vec4(vec3(0), 1.0 / 0.0));
    // imageStore(hitMaterial, texelCoord, uvec4(0));
}
