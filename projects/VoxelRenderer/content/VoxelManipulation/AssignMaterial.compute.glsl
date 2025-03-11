#version 460 core

layout(local_size_x = 4, local_size_y = 8, local_size_z = 8) in;

layout(std430, binding = 0) buffer MaterialMap
{
    uint materialMap[];
};

uniform ivec3 voxelCount;

uint getMaterialIndex(ivec3 voxelPosition)
{
    int i16Index = voxelPosition.x + voxelCount.x * (voxelPosition.y + voxelCount.y * voxelPosition.z);
    int i32Index = i16Index / 2;
    int bitsShifted = (i16Index & 1) * 16;

    return (materialMap[i32Index] & (0xffff << bitsShifted)) >> bitsShifted;
}

void setMaterialIndex(ivec3 voxelPosition, uint materialIndex)
{
    int i16Index = voxelPosition.x + voxelCount.x * (voxelPosition.y + voxelCount.y * voxelPosition.z);
    int i32Index = i16Index / 2;
    int bitsShifted = (i16Index & 1) * 16;

    materialMap[i32Index] &= ~(0xffff << bitsShifted);
    materialMap[i32Index] |= materialIndex << bitsShifted;
}

void main()
{
    ivec3 voxelPosition = ivec3(gl_GlobalInvocationID.xyz);
    voxelPosition.x *= 2; // We set 2 voxels along the x-axis each time due to material IDs being 16-bits and GLSL only being able to address 32-bits

    int i16Index = (voxelPosition.x + voxelCount.x * (voxelPosition.y + voxelCount.y * voxelPosition.z));
    int i32Index = i16Index / 2;

    materialMap[i32Index] = (1 << 16) | (2 << 0); // TODO: Create a more interesting pattern
}
