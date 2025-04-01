#version 460 core

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

uniform ivec3 resolution; //(xSize, ySize, raysPerPixel)

uniform float maxDepth;

layout(std430, binding = 0) buffer RayMisc
{
    float rayMisc[];
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


void main()
{
    ivec3 texelCoord = ivec3(gl_GlobalInvocationID.xyz);

    
    if(getRayDepth(texelCoord) > 0){
        setRayDepth(texelCoord, maxDepth);
    }
}
