#version 440 core

layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;

layout(std430, binding = 0) buffer MaterialMap
{
    uint materialMap[];
};

uniform ivec3 resolution; //(xSize, ySize, zSize) size of the texture being set (Not the number of voxels, it is the number of cells)
uniform uint materialStartIndex;

void setByte(ivec3 coord, uint value, uint byteNumber)
{
    int index = (coord.x + resolution.x * (coord.y + resolution.y * coord.z)) + int(materialStartIndex);
    
    int bufferIndex = index;//4 bits are used for a single material and these bits are spread across 4 bytes, so the index of the cell is the index of the uint
    int bufferOffset = int(byteNumber);


    materialMap[bufferIndex] &= ~(uint(255) << (8 * bufferOffset));
    materialMap[bufferIndex] |= value << (8 * bufferOffset);
}

void main()
{

    ivec3 texelCoord = ivec3(gl_GlobalInvocationID.xyz);
    ivec3 pos = 2 * texelCoord;

    uvec3 material = uvec3(0); // This is the bit packed value

    int k = 1;

    for (int i = 0; i < 2; i++) // z axis
    {
        for (int j = 0; j < 2; j++) // y axis
        {
            for (int l = 0; l < 2; l++) // x axis
            {

                ivec3 voxelPos = pos + ivec3(l, j, i);
                // This is for a single voxel
                // This sets the 3 bits of the material stored in this mip map
                // The least significant bit is stored in r, then g, then b
                // And lower mip map levels store bits of lower significance
                uvec3 materialDecider = uvec3(mod(voxelPos.x, 2), mod(voxelPos.y, 2), mod(voxelPos.z, 2));

                material |= k * materialDecider;

                // material = uvec3(material.r | (k * materialDecider.r), material.g | (k * materialDecider.g), material.b | (k * materialDecider.b));
                k = k << 1;
            }
        }
    }

    setByte(texelCoord, material.r, 0);
    setByte(texelCoord, material.g, 1);
    setByte(texelCoord, material.b, 2);
    setByte(texelCoord, 0, 3);
}
