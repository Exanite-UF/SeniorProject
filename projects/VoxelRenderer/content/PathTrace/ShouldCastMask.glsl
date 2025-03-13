#version 460 core

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;


uniform ivec3 resolution; //(xSize, ySize, raysPerPixel)

//These actually need to be passed in with a ssbo
//All the voxel worlds are processed at the same time
uniform vec3 voxelWorldPosition;
uniform vec4 voxelWorldRotation; // This is a quaternion
uniform vec3 voxelWorldScale; // Size of a voxel
uniform uint worldNumber;

layout(std430, binding = 0) buffer RayPosition
{
    coherent restrict float rayPosition[];
};

vec3 getRayPosition(ivec3 coord)
{
    int index = 3 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride of 3, axis order is x y z
    return vec3(rayPosition[0 + index], rayPosition[1 + index], rayPosition[2 + index]);
}

layout(std430, binding = 1) buffer RayDirection
{
    coherent restrict float rayDirection[];
};

vec3 getRayDirection(ivec3 coord)
{
    int index = 3 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride of 3, axis order is x y

    return vec3(rayDirection[0 + index], rayDirection[1 + index], rayDirection[2 + index]);
}


layout(std430, binding = 2) buffer ShouldCastMask
{
    readonly uint showCastMask[];
};

//Disables casting for all world numbers
void clearShouldCast(ivec3 coord){
    int index = (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride of 1, axis order is x y z

    showCastMask[index] = 0;
}

//if value is true, it enables casting for the provided world number
//If value is false, it does nothing
void changeShouldCast(ivec3 coord, uint worldNumber, bool value){
    int index = (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride of 1, axis order is x y z

    showCastMask[index] |= (uint(value) << (worldNumber % 32));
}