#version 440 core

layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

// layout(rgba8ui, binding = 0) uniform uimage3D imgInput;
// layout(rgba8ui, binding = 1) uniform uimage3D imgOutput;

// The shader is invoked once per byte in the next mipmap level

layout(std430, binding = 0) buffer OccupancyMap
{
    uint occupancyMap[];
};

uniform ivec3 resolution; //(xSize, ySize, zSize) size of the previous mipMap texture
uniform uint previousStartByte; // The index of the first byte of the previous mipMap level
uniform uint nextStartByte; // The index of the first byte of the next mipMap level (The one we are making)

// Sets a byte in the next mipmap
void setByte(ivec3 coord, uint value)
{
    ivec3 tempRes = resolution / 4; // The next mipmap texture has a size 4 times smaller on each axis
    int index = (coord.x + tempRes.x * (coord.y + tempRes.y * coord.z)) + int(nextStartByte);
    int bufferIndex = index / 4; // Divide by 4, because glsl does not support single byte data types, so a 4 byte data type is being used
    int bufferOffset = (index & 3); // Modulus 4 done using a bitmask


    occupancyMap[bufferIndex] &= ~(uint(255) << (8 * bufferOffset));
    occupancyMap[bufferIndex] |= value << (8 * bufferOffset);
}

// Reads a byte from the previous mipmap
uint getByte(ivec3 coord)
{
    int index = (coord.x + resolution.x * (coord.y + resolution.y * coord.z)) + int(previousStartByte);
    int bufferIndex = index / 4; // Divide by 4, because glsl does not support single byte data types, so a 4 byte data type is being used
    int bufferOffset = (index & 3); // Modulus 4 done using a bitmask

    return (occupancyMap[bufferIndex] & (255 << (8 * bufferOffset))) >> (8 * bufferOffset);
}

//This calculates the value that should be stored in a single byte of the next mip map level
void makeMipMap(ivec3 cellCoord)
{
    ivec3 texelCoord = cellCoord; // texel coord in the position in the next mipmap level

    ivec3 pos = 4 * texelCoord;

    uint resultMask = 0;
    int bit = 1;
    for (int zCurr = 0; zCurr < 2; zCurr++) // z axis
    {
        for (int yCurr = 0; yCurr < 2; yCurr++) // y axis
        {
            for (int xCurr = 0; xCurr < 2; xCurr++) // x axis
            {
                // pos + (l, j, i). Tell us the position of the bitlevel cell that we are in for this mip map.
                // We need to sample a 2x2x2 set of cells in the previous mip map
                // If any are occupied then we have an occupied cell
                ivec3 cellPos = pos + 2 * ivec3(xCurr, yCurr, zCurr);

                bool isOccupied = false; // This is the result of the occupancy search
                for (int zPrev = 0; zPrev < 2 && !isOccupied; zPrev++)
                {
                    for (int yPrev = 0; yPrev < 2 && !isOccupied; yPrev++)
                    {
                        for (int xPrev = 0; xPrev < 2 && !isOccupied; xPrev++)
                        {
                            ivec3 subCellPos = cellPos + ivec3(xPrev, yPrev, zPrev); // This is the position of a cell in the previous mip map
                            uint value = getByte(subCellPos); // We check to see if that cell is occupied

                            // If the previous mipmap's mask is not 0, at least one voxel in the previous mipmap is occupied
                            isOccupied = value != 0;
                        }
                    }
                }

                if (isOccupied)
                {
                    resultMask |= bit;
                }
                bit = bit << 1;
            }
        }
    }

    setByte(texelCoord, resultMask);
}

void main()
{
    uint uintIndex = gl_GlobalInvocationID.x;

    uvec3 nextRes = resolution / 4;//This is the resolution of the mipmap being generated
    
    for (int byteOffset = 0; byteOffset < 4; byteOffset++)
    {
        // Each uint is 4 bytes, where each byte contains a 2x2x2 group of voxels
        uint byteIndex = uintIndex * 4 + byteOffset;

        // Convert byte index into the voxel group's base position
        uint baseX = byteIndex % nextRes.x;
        byteIndex /= nextRes.x;
        uint baseY = byteIndex % nextRes.y;
        byteIndex /= nextRes.y;
        uint baseZ = byteIndex;

        ivec3 basePosition = ivec3(baseX, baseY, baseZ);

        makeMipMap(basePosition);//Call the make mip map function for each byte of the uint
    }
}
