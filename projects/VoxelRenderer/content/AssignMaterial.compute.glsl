#version 460 core

layout(local_size_x = 1024, local_size_y = 1, local_size_z = 1) in;

layout(std430, binding = 0) buffer MaterialMap
{
    uint materialMap[];
};

uniform uint elementCount;

void main()
{
    uint index = gl_GlobalInvocationID.x;
    if (index >= elementCount)
    {
        return;
    }

    // The original AssignMaterial shader had the effect of setting all indices to 0x00f0ccaa
    materialMap[index] = 0x00f0ccaa;
    // materialMap[index] = 0x76543210;
    // materialMap[index] = index;
}
