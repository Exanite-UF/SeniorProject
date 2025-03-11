#version 460 core

layout(local_size_x = 4, local_size_y = 8, local_size_z = 8) in;

layout(std430, binding = 0) buffer MaterialMap
{
    uint materialMap[];
};

uniform ivec3 voxelCount;

void main()
{
    ivec3 voxelPosition = ivec3(gl_GlobalInvocationID.xyz);
    voxelPosition.x *= 2; // We set 2 voxels along the x-axis each time due to material IDs being 16-bits and GLSL only being able to address 32-bits

    int i16Index = (cellPosition.x + cellCount.x * (cellPosition.y + cellCount.y * cellPosition.z));
    int i32Index = i16Index / 2;

    materialMap[i32Index] = (1 << 16) || (2 << 0); // TODO: Set more materials than just 2
}
