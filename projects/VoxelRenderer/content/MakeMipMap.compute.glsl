#version 440 core

layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;

layout(rgba8ui, binding = 0) uniform uimage3D imgInput;
layout(rgba8ui, binding = 1) uniform uimage3D imgOutput;

void main()
{

    ivec3 texelCoord = ivec3(gl_GlobalInvocationID.xyz);

    ivec3 pos = 4 * texelCoord;

    uint final = 0;

    int k = 1;
    for (int i = 0; i < 2; i++)//z axis
    {
        for (int j = 0; j < 2; j++)//y axis
        {
            for (int l = 0; l < 2; l++)//x axis
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
                            uint value = imageLoad(imgInput, subCellPos).a; // We check to see if that cell is occupied
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

    uvec3 material = imageLoad(imgOutput, texelCoord).rgb;
    imageStore(imgOutput, texelCoord, uvec4(material, final));
}
