#version 460 core

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;


uniform ivec3 resolution; //(xSize, ySize, raysPerPixel)
uniform bool resetLight;
uniform bool resetAttentuation;
uniform bool currentBuffer;


layout(std430, binding = 0) buffer RayPosition
{
    coherent restrict float rayPosition[];
};

vec3 getRayPosition(ivec3 coord)
{
    int index = 3 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride of 3, axis order is x y z
    return vec3(rayPosition[0 + index], rayPosition[1 + index], rayPosition[2 + index]);
}

layout(std430, binding = 1) buffer RayDirection
{
    coherent restrict float rayDirection[];
};

vec3 getRayDirection(ivec3 coord)
{
    int index = 3 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride of 3, axis order is x y

    return vec3(rayDirection[0 + index], rayDirection[1 + index], rayDirection[2 + index]);
}

layout(std430, binding = 2) buffer PriorAttenuation1
{
    float priorAttenuation1[];
};

layout(std430, binding = 3) buffer AccumulatedLight1
{
    float accumulatedLight1[];
};

layout(std430, binding = 4) buffer PriorAttenuation2
{
    float priorAttenuation2[];
};

layout(std430, binding = 5) buffer AccumulatedLight2
{
    float accumulatedLight2[];
};

layout(std430, binding = 6) buffer FirstHitNormal
{
    float firstHitNormal[];
};

layout(std430, binding = 7) buffer FirstHitPosition
{
    float firstHitPosition[];
};

layout(std430, binding = 8) buffer FirstHitMaterial
{
    writeonly float firstHitMaterial[];
};

void setFirstHitMaterial(ivec3 coord, vec3 value)
{
    int index = 3 * (coord.x + resolution.x * (coord.y)); // Stride of 3, axis order is x y z
    firstHitMaterial[0 + index] = value.x;
    firstHitMaterial[1 + index] = value.y;
    firstHitMaterial[2 + index] = value.z;
}


void setAttenuation(ivec3 coord, vec3 value)
{
    int index = 3 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride is 3, axis order is x y z

    priorAttenuation1[0 + index] = value.x;
    priorAttenuation1[1 + index] = value.y;
    priorAttenuation1[2 + index] = value.z;

    priorAttenuation2[0 + index] = value.x;
    priorAttenuation2[1 + index] = value.y;
    priorAttenuation2[2 + index] = value.z;
}

void setLightAccumulation(ivec3 coord, vec3 value)
{
    int index = 3 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride of 3, axis order is x y z
    int temp = currentBuffer ? 1 : 0;
    int temp2 = 1 - temp;

    accumulatedLight1[0 + index] = value.x * temp2;
    accumulatedLight1[1 + index] = value.y * temp2;
    accumulatedLight1[2 + index] = value.z * temp2;

    accumulatedLight2[0 + index] = value.x * temp;
    accumulatedLight2[1 + index] = value.y * temp;
    accumulatedLight2[2 + index] = value.z * temp;
}

void setFirstHitNormal(ivec3 coord, vec3 value)
{
    int index = 3 * (coord.x + resolution.x * (coord.y)); // Stride of 3, axis order is x y z
    firstHitNormal[0 + index] = value.x;
    firstHitNormal[1 + index] = value.y;
    firstHitNormal[2 + index] = value.z;
}

void setFirstHitPosition(ivec3 coord, vec3 value)
{
    int index = 3 * (coord.x + resolution.x * (coord.y)); // Stride of 3, axis order is x y z
    firstHitPosition[0 + index] = value.x;
    firstHitPosition[1 + index] = value.y;
    firstHitPosition[2 + index] = value.z;
}

float sunSize = 0.99;
vec3 sunDir = normalize(vec3(1, -1, 1));
float sunBrightness = 5;

void main()
{
    ivec3 texelCoord = ivec3(gl_GlobalInvocationID.xyz);

    vec3 startPos = getRayPosition(texelCoord);
    vec3 rayDir = normalize(getRayDirection(texelCoord));

    
    //setAttenuation(texelCoord, vec3(0));


    if (resetAttentuation)
    {
        setAttenuation(texelCoord, vec3(1));
    }

    if (resetLight)
    {
        if (dot(rayDir, sunDir) > sunSize)
        {
            setLightAccumulation(texelCoord, sunBrightness / (6.28318530718 * (1 - sunSize)) * vec3(1, 1, 1));
        }
        else if (dot(rayDir, vec3(0, 0, 1)) > 0)
        {
            setLightAccumulation(texelCoord, 1 * vec3(40, 77, 222) / 255);
        }
        else
        {
            setLightAccumulation(texelCoord, 0.1 * vec3(61, 150, 11) / 255);
        }
    }

    if (texelCoord.z == 0)
    {
        setFirstHitPosition(texelCoord, startPos + rayDir * 100000);
        setFirstHitMaterial(texelCoord, vec3(0, 0, 0)); // The skybox has a roughness of 0
        setFirstHitNormal(texelCoord, rayDir); // The skybox has a roughness of 0
    }

    // imageStore(hitPosition, texelCoord, vec4(0));
    // imageStore(hitNormal, texelCoord, vec4(vec3(0), 1.0 / 0.0));
    // imageStore(hitMaterial, texelCoord, uvec4(0));
}
