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

uint sampleMaterialIndex(ivec3 voxelPosition)
{
    ivec3 size2 = voxelCount >> 4;

    ivec3 position0 = voxelPosition;
    ivec3 position1 = position0 >> 2;
    ivec3 position2 = position0 >> 4;

    uint materialBits0 = ((position0.z & 1) << 2) | ((position0.y & 1) << 1) | ((position0.x & 1) << 0);
    uint materialBits1 = ((position1.z & 1) << 2) | ((position1.y & 1) << 1) | ((position1.x & 1) << 0);
    uint materialBits2 = ((position2.z & 1) << 2) | ((position2.y & 1) << 1) | ((position2.x & 1) << 0);

    uint materialOffset = position2.x + size2.x * (position2.y + size2.y * position2.z);

    uint materialIndex = ((materialBits2 << 6) | (materialBits1 << 3) | (materialBits0 << 0)) + materialOffset;

    return materialIndex % 65536;
}

void main()
{
    ivec3 voxelPosition = ivec3(gl_GlobalInvocationID.xyz);
    voxelPosition.x *= 2; // We set 2 voxels along the x-axis each time due to material IDs being 16-bits and GLSL only being able to address 32-bits

    int i16Index = (voxelPosition.x + voxelCount.x * (voxelPosition.y + voxelCount.y * voxelPosition.z));
    int i32Index = i16Index / 2;

    setMaterialIndex(ivec3(voxelPosition.x, voxelPosition.y, voxelPosition.z), sampleMaterialIndex(ivec3(voxelPosition.x, voxelPosition.y, voxelPosition.z)));
    setMaterialIndex(ivec3(voxelPosition.x + 1, voxelPosition.y, voxelPosition.z), sampleMaterialIndex(ivec3(voxelPosition.x + 1, voxelPosition.y, voxelPosition.z)));
}
