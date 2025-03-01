#version 460 core

layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;

layout(std430, binding = 0) buffer MaterialMap
{
    uint materialMap[];
};

uniform ivec3 cellCount; //(xSize, ySize, zSize) size of the texture being set (Not the number of voxels, it is the number of cells)
uniform uint materialStartIndex;

void main()
{
    ivec3 cellPosition = ivec3(gl_GlobalInvocationID.xyz);
    int index = (cellPosition.x + cellCount.x * (cellPosition.y + cellCount.y * cellPosition.z)) + int(materialStartIndex / 4);

    // The original AssignMaterial shader had the effect of setting all indices to 0x00f0ccaa
    // materialMap[index] = index;
    materialMap[index] = 0x00f0ccaa;
    // materialMap[index] = 0x76543210;
    // materialMap[index] = 0x01234567;
}
