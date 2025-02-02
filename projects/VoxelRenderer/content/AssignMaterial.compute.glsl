#version 440 core

layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;

layout(rgba8ui, binding = 0) uniform uimage3D img;

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

    imageStore(img, texelCoord, uvec4(material, imageLoad(img, texelCoord).a));
}
