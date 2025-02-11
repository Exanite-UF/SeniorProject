#version 460 core

layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

// layout(rgba8ui, binding = 0) uniform uimage3D imgOutput;

layout(std430, binding = 0) buffer OccupancyMap
{
    uint occupancyMap[];
};

uniform ivec3 resolution; // Size of the occupancy map in voxels
uniform float time;
uniform bool isRand2;
uniform float fillAmount;

float rand(vec3 x)
{
    return fract(sin(dot(x, vec3(12.9898, 78.233, 21.6452))) * 43758.5453 + time);
}

float wave(float x)
{
    return sin(x) * 0.5 + 0.5;
}

float rand2(vec3 x)
{
    float result = 0;
    result += wave(0.0173651103013 * dot(x, vec3(0.587850564235, 0.758284772211, 0.43462848748)) + time);
    result += wave(0.0685989677893 * dot(x, vec3(-0.376693290124, 0.0670154499555, 0.0673054752805)) + time);
    result += wave(0.0117508870384 * dot(x, vec3(0.2477844726, 0.412264222346, 0.561235509959)) + time);
    result += wave(0.05551583823 * dot(x, vec3(0.280613371903, -0.566017829842, 0.581708656338)) + time);
    return result / 4;
}

void setByte(ivec3 coord, uint value)
{
    int index = (coord.x + resolution.x * (coord.y + resolution.y * coord.z));
    int bufferIndex = index / 4; // Divide by 4, because glsl does not support single byte data types, so a 4 byte data type is being used
    int bufferOffset = (index & 3); // Modulus 4 done using a bitmask

    occupancyMap[bufferIndex] &= (uint(255) << (8 * (3 - bufferOffset))) ^ 0xFFFFFFFFu; // Create a mask that zeros out the part we want to set
    occupancyMap[bufferIndex] |= value << (8 * (3 - bufferOffset));
}

uint getByte(ivec3 coord)
{
    int index = (coord.x + resolution.x * (coord.y + resolution.y * coord.z));
    int bufferIndex = index / 4; // Divide by 4, because glsl does not support single byte data types, so a 4 byte data type is being used
    int bufferOffset = (index & 3); // Modulus 4 done using a bitmask

    // return occupancyMap[index];
    return uint(occupancyMap[bufferIndex] & (255 << (8 * (3 - bufferOffset)))) >> (8 * (3 - bufferOffset)); // Mask for the section we want then put it in the least signigicant bits
}


void makeNoise(ivec3 cellCoord){
    ivec3 texelCoord = cellCoord;

    vec3 pos = 2 * texelCoord;

    uint final = 0;
    if (isRand2)
    {
        int k = 1;
        for (int i = 0; i < 2; i++) // z axis
        {
            for (int j = 0; j < 2; j++) // y axis
            {
                for (int l = 0; l < 2; l++) // x axis
                {
                    float value = 0;
                    vec3 temp = pos + vec3(l, j, i);
                    value = rand2(temp);
                    value += rand2(floor(temp / 2 + 1));
                    value += rand2(floor(temp / 4 + 2));
                    value += rand2(floor(temp / 8 + 3));
                    value /= 4;
                    if (value > fillAmount && value < fillAmount + 0.01)
                    { // This is how you set the density
                        final |= k;
                    }
                    k = k << 1;
                }
            }
        }
    }
    else
    {
        int k = 1;
        for (int i = 0; i < 2; i++) // z axis
        {
            for (int j = 0; j < 2; j++) // y axis
            {
                for (int l = 0; l < 2; l++) // x axis
                {
                    float value = 0;
                    vec3 temp = pos + vec3(l, j, i);
                    value = rand(temp);
                    value += rand(floor(temp / 2 + 1));
                    value += rand(floor(temp / 4 + 2));
                    value += rand(floor(temp / 8 + 3));
                    value /= 4;
                    if (value > fillAmount)
                    { // This is how you set the density
                        final |= k;
                    }
                    k = k << 1;
                }
            }
        }
    }

    setByte(texelCoord, final);
}

void main()
{
    uint uintIndex = gl_GlobalInvocationID.x;

    for (int byteOffset = 0; byteOffset < 4; byteOffset++)
    {
        // Each uint is 4 bytes, where each byte contains a 2x2x2 group of voxels
        uint byteIndex = uintIndex * 4 + byteOffset;

        // Convert byte index into the voxel group's base position
        uint baseX = byteIndex % resolution.x;
        byteIndex /= resolution.x;
        uint baseY = byteIndex % resolution.y;
        byteIndex /= resolution.y;
        uint baseZ = byteIndex;

        ivec3 basePosition = ivec3(baseX, baseY, baseZ);

        makeNoise(basePosition);
    }

}
