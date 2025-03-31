#version 460 core

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

uniform ivec3 resolution; //(xSize, ySize, raysPerPixel)

uniform float maxDepth;

layout(std430, binding = 0) buffer RayMisc
{
    float rayMisc[];
};

layout(std430, binding = 1) buffer RayPixel
{
    int rayPixel[];
};


void setRayDepth(ivec3 coord, float value)
{
    int index = 1 * (coord.x + resolution.x * coord.y); // axis order is x y
    rayMisc[index + 0] = value;
}

float getRayDepth(ivec3 coord)
{
    int index = 1 * (coord.x + resolution.x * coord.y); // axis order is x y
    return rayMisc[index + 0];
}

ivec3 getRayPixel(ivec3 coord)
{
    int index = 2 * (coord.x + resolution.x * coord.y); // Stride of 3, axis order is x y z
    return ivec3(rayPixel[0 + index], rayPixel[1 + index], 0);
}

void main()
{
    ivec3 texelCoord = getRayPixel(ivec3(gl_GlobalInvocationID.xyz));

    if(getRayDepth(texelCoord) > 0){
        setRayDepth(texelCoord, maxDepth);
    }
}
