#version 460 core
layout(r32ui, binding = 0) uniform uimage3D voxelTexture;

uniform vec3 gridSize;
uniform vec3 minBounds;
uniform vec3 maxBounds;

in vec3 worldPosition;

void main()
{
    vec3 normalizedPos = (worldPosition - minBounds) / (maxBounds - minBounds);

    ivec3 voxelCoord = ivec3(normalizedPos * gridSize);

    if (all(greaterThanEqual(voxelCoord, ivec3(0))) && all(lessThan(voxelCoord, ivec3(gridSize)))) {
        imageAtomicAdd(voxelTexture, voxelCoord, 1);
    }

}

