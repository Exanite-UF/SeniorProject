#version 460 core

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
//layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

uniform ivec3 resolution; //(xSize, ySize, 1)

uniform float maxDepth;
uniform bool shouldDrawSkybox;

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



layout(std430, binding = 1) buffer RayDirection
{
    float rayDirection[];
};

vec3 getRayDirection(ivec3 coord)
{
    int index = 3 * (coord.x + resolution.x * coord.y); // Stride of 3, axis order is x y

    return vec3(rayDirection[0 + index], rayDirection[1 + index], rayDirection[2 + index]);
}


layout(std430, binding = 2) buffer AccumulatedLightIn
{
    restrict float accumulatedLightIn[];
};


layout(std430, binding = 3) buffer AccumulatedLightOut
{
    restrict float accumulatedLightOut[];
};

layout(std430, binding = 4) buffer AttenuationIn
{
    restrict float attenuationIn[];
};

vec3 getPriorAttenuation(ivec3 coord)
{
    int index = 3 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride is 3, axis order is x y z

    return vec3(attenuationIn[index + 0], attenuationIn[index + 1], attenuationIn[index + 2]);
}

void changeLightAccumulation(ivec3 coord, vec3 deltaValue)
{
    int index = 3 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride of 3, axis order is x y z
    accumulatedLightOut[0 + index] = accumulatedLightIn[0 + index] + deltaValue.x;
    accumulatedLightOut[1 + index] = accumulatedLightIn[1 + index] + deltaValue.y;
    accumulatedLightOut[2 + index] = accumulatedLightIn[2 + index] + deltaValue.z;
}

uniform float sunAngularSize; // The angle of the sun in diameter
float sunSize = cos(sunAngularSize * 3.14159265 / 180.0);
uniform vec3 sunDir;
uniform float sunBrightness;
uniform vec3 skyColor;
uniform vec3 groundColor;

vec3 skyBox(vec3 rayDirection){
    if(dot(normalize(sunDir), normalize(rayDirection)) > sunSize){
        return sunBrightness / (6.28318530718 * (1 - sunSize)) * vec3(1, 1, 1);
    }else if(dot(rayDirection, vec3(0, 0, 1)) > 0){
        return skyColor;
    }else{
        return groundColor;
    }
}


void main()
{
    ivec3 texelCoord = ivec3(gl_GlobalInvocationID.xyz);

    float depth = getRayDepth(texelCoord);

    if(depth == maxDepth){
        if(shouldDrawSkybox){
            changeLightAccumulation(texelCoord, skyBox(getRayDirection(texelCoord)) * getPriorAttenuation(texelCoord));
        }

        setRayDepth(texelCoord, -1);
    }
}
