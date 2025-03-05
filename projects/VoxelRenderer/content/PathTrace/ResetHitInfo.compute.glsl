#version 460 core

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(std430, binding = 0) buffer HitMisc
{
    float hitMisc[];
};

uniform ivec3 resolution; //(xSize, ySize, raysPerPixel)

void setHitWasHit(ivec3 coord, bool value)
{
    int index = 2 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // axis order is x y z
    hitMisc[index + 0] = (value) ? 1.0 : 0.0;
}

void setHitDist(ivec3 coord, float value)
{
    int index = 2 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // axis order is x y z
    hitMisc[index + 1] = value;
}

// layout(rgba32f, binding = 0) uniform writeonly image3D hitPosition;
// layout(rgba32f, binding = 1) uniform writeonly image3D hitNormal;
// layout(r16ui, binding = 2) uniform writeonly uimage3D hitMaterial;

void main()
{
    ivec3 texelCoord = ivec3(gl_GlobalInvocationID.xyz);

    setHitWasHit(texelCoord, false);
    setHitDist(texelCoord, 1.0 / 0.0);
}
