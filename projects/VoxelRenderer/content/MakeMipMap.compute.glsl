#version 440 core

layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;

//layout(rgba8ui, binding = 0) uniform uimage3D imgInput;
//layout(rgba8ui, binding = 1) uniform uimage3D imgOutput;

//The shader is invoked once per byte in the next mipmap level

layout(std430, binding = 0) buffer OccupancyMap{
    uint occupancyMap[];
};

uniform ivec3 resolution;//(xSize, ySize, zSize) size of the previous mipMap texture
uniform uint previousStartByte;//The index of the first byte of the previous mipMap level
uniform uint nextStartByte;//The index of the first byte of the next mipMap level (The one we are making)

//Sets a byte in the next mipmap
void setByte(ivec3 coord, uint value){
    ivec3 tempRes = resolution / 4;//The next mipmap texture has a size 4 times smaller on each axis
    int index = (coord.x + tempRes.x * (coord.y + tempRes.y * coord.z)) + int(nextStartByte);
    int bufferIndex = index / 4;//Divide by 4, because glsl does not support single byte data types, so a 4 byte data type is being used
    int bufferOffset = (index & 3);//Modulus 4 done using a bitmask

    //occupancyMap[bufferIndex] &= ~(uint(0xff000000) >> bufferOffset);//Create a mask that zeros out the part we want to set
    if(bufferOffset == 0){
        occupancyMap[bufferIndex] &= -(uint(255) << (8 * 3)) - 1;//(uint(255) << (8 * 3)) ^ 0xFFFFFFFFu;//Create a mask that zeros out the part we want to set
        occupancyMap[bufferIndex] |= value << (8 * 3);
    }
    if(bufferOffset == 1){
        occupancyMap[bufferIndex] &= -(uint(255) << (8 * 2)) - 1;//(uint(255) << (8 * 2)) ^ 0xFFFFFFFFu;//Create a mask that zeros out the part we want to set
        occupancyMap[bufferIndex] |= value << (8 * 2);
    }
    if(bufferOffset == 2){
        occupancyMap[bufferIndex] &= -(uint(255) << (8 * 1)) - 1;;//(uint(255) << (8 * 1)) ^ 0xFFFFFFFFu;//Create a mask that zeros out the part we want to set
        occupancyMap[bufferIndex] |= value << (8 * 1);
    }
    if(bufferOffset == 3){
        occupancyMap[bufferIndex] &= -(uint(255) << (8 * 0)) - 1;//(uint(255) << (8 * 0)) ^ 0xFFFFFFFFu;//Create a mask that zeros out the part we want to set
        occupancyMap[bufferIndex] |= value << (8 * 0);
    }
}

//Reads a byte from the previous mipmap
uint getByte(ivec3 coord){
    int index = (coord.x + resolution.x * (coord.y + resolution.y * coord.z)) + int(previousStartByte);
    int bufferIndex = index / 4;//Divide by 4, because glsl does not support single byte data types, so a 4 byte data type is being used
    int bufferOffset = (index & 3);//Modulus 4 done using a bitmask

    if(bufferOffset == 0){
        return (occupancyMap[bufferIndex] & (255 << (8 * (3 - 0)))) >> (8 * (3 - 0));
    }
    if(bufferOffset == 1){
        return (occupancyMap[bufferIndex] & (255 << (8 * (3 - 1)))) >> (8 * (3 - 1));
    }
    if(bufferOffset == 2){
        return (occupancyMap[bufferIndex] & (255 << (8 * (3 - 2)))) >> (8 * (3 - 2));
    }
    if(bufferOffset == 3){
        return (occupancyMap[bufferIndex] & (255 << (8 * (3 - 3)))) >> (8 * (3 - 3));
    }
    return 0;
    //return (occupancyMap[bufferIndex] & (255 << (8 * (3 - bufferOffset)))) >> (8 * (3 - bufferOffset));//Mask for the section we want then put it in the least signigicant bits
}

void main()
{

    ivec3 texelCoord = ivec3(gl_GlobalInvocationID.xyz);//texel coord in the position in the next mipmap level

    ivec3 pos = 4 * texelCoord;

    uint final = 0;

    int k = 1;
    for (int i = 0; i < 2; i++) // z axis
    {
        for (int j = 0; j < 2; j++) // y axis
        {
            for (int l = 0; l < 2; l++) // x axis
            {
                // pos + (l, j, i). Tell us the position of the bitlevel cell that we are in for this mip map.
                // We need to sample a 2x2x2 set of cells in the previous mip map
                // If any are occupied then we have an occupied cell
                ivec3 cellPos = pos + 2 * ivec3(l, j, i);

                bool isOccupied = false; // This is the result of the occupancy search
                for (int i2 = 0; i2 < 2 && !isOccupied; i2++)
                {
                    for (int j2 = 0; j2 < 2 && !isOccupied; j2++)
                    {
                        for (int l2 = 0; l2 < 2 && !isOccupied; l2++)
                        {
                            ivec3 subCellPos = cellPos + ivec3(l2, j2, i2); // This is the position of a cell in the previous mip map
                            uint value = getByte(subCellPos); // We check to see if that cell is occupied
                            if (value != 0)
                            { // If it is then we know that the cell in the currently generated mip map is also full
                                isOccupied = true;
                            }
                        }
                    }
                }

                if (isOccupied)
                {
                    final |= k;
                }
                k = k << 1;
            }
        }
    }

    setByte(texelCoord, final);
}
