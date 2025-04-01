#version 460 core

//layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

uniform ivec3 resolution; //(xSize, ySize, 1)


layout(std430, binding = 0) buffer RayPixelIn
{
    int rayPixelIn[];
};

layout(std430, binding = 1) buffer RayPixelOut
{
    int rayPixelOut[];
};

ivec3 getRayPixel(ivec3 coord)
{
    int index = 2 * (coord.x + resolution.x * coord.y); // Stride of 2, axis order is x y z
    return ivec3(rayPixelIn[0 + index], rayPixelIn[1 + index], 0);
}

void setRayPixel(int index, ivec2 value){
    rayPixelOut[2 * index + 0] = value.x;
    rayPixelOut[2 * index + 1] = value.y;
}

void setRayPixel(ivec3 coord, ivec2 value){
    int index = 2 * (coord.x + resolution.x * coord.y); // Stride of 2, axis order is x y z
    rayPixelOut[index + 0] = value.x;
    rayPixelOut[index + 1] = value.y;
}

shared int accessCounter;
shared int counter;
shared int backCounter;

void main()
{
    accessCounter = 0;
    counter = 0;
    backCounter = 1;

    int pixelCount = resolution.x * resolution.y * resolution.z;

    uint numWorkGroups = gl_NumWorkGroups.x * gl_NumWorkGroups.y * gl_NumWorkGroups.z;
    int workGroupID = int(gl_WorkGroupID.x + gl_NumWorkGroups.x * (gl_WorkGroupID.y + gl_NumWorkGroups.y * gl_WorkGroupID.z));//Offset

    uint localID = gl_LocalInvocationID.x + gl_WorkGroupSize.x * (gl_LocalInvocationID.y + gl_WorkGroupSize.y * gl_LocalInvocationID.z);

    int stride = int(numWorkGroups);//uint(ceil(float(pixelCount) / numWorkGroups));

    ivec3 texelCoord = getRayPixel(ivec3(gl_GlobalInvocationID.xyz));


    bool isInvalid = texelCoord.x < 0;

    while(true){
        barrier();
///
        if(accessCounter >= gl_WorkGroupSize.x * gl_WorkGroupSize.y * gl_WorkGroupSize.z){
            break;
        }
        if(accessCounter == localID){
            int index = 0;
            if(isInvalid){
                index = pixelCount - backCounter * stride + workGroupID;
                backCounter++;
            }else{
                index = counter * stride + workGroupID;
                counter++;
            }
            
            //index = accessCounter * stride + workGroupID;
///
            if(index >= pixelCount || index < 0){
                accessCounter++;
                return;
            }
///
            
        
            //setRayPixel(int(index), ivec2(-1));
            //setRayPixel(int(index), ivec2(accessCounter, localID));
            if(isInvalid){
                setRayPixel(int(index), ivec2(-1));
            }else{
                setRayPixel(int(index), texelCoord.xy);
                //setRayPixel(int(index), ivec2(counter, workGroupID));
            }
            
            
            accessCounter++;
        }
    }
}
