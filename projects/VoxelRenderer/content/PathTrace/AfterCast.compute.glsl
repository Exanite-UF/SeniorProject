#version 460 core

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

uniform ivec3 resolution; //(xSize, ySize, 1)

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

void setRayPixel(int index, ivec2 value){
    rayPixel[2 * index + 0] = value.x;
    rayPixel[2 * index + 1] = value.y;
}


shared uint accessCounter = 0;
shared uint counter = 0;

void main()
{
    uint pixelCount = resolution.x * resolution.y * resolution.z;

    uint numWorkGroups = gl_NumWorkGroups.x * gl_NumWorkGroups.y * gl_NumWorkGroups.z;
    uint workGroupID = gl_WorkGroupID.x + gl_NumWorkGroups.x * (gl_WorkGroupID.y + gl_NumWorkGroups.y * gl_WorkGroupID.z);//Offset

    uint localID = gl_LocalInvocationID.x + gl_WorkGroupSize.x * (gl_LocalInvocationID.y + gl_WorkGroupSize.y * gl_LocalInvocationID.z);

    uint stride = uint(ceil(float(pixelCount) / numWorkGroups));

    ivec3 texelCoord = getRayPixel(ivec3(gl_GlobalInvocationID.xyz));

    if(getRayDepth(texelCoord) == maxDepth){
        setRayDepth(texelCoord, -1);
    }else{
        //Reorder ray pixel
        while(accessCounter != localID){}

        uint index = counter * stride + workGroupID;
        if(index >= pixelCount){
            return;
        }
        setRayPixel(int(index), texelCoord.xy);
        accessCounter++;
    }
}
