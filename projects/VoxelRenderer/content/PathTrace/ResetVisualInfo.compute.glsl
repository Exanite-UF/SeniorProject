#version 460 core

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

uniform ivec3 resolution; //(xSize, ySize, raysPerPixel)
uniform bool resetLight;
uniform bool drawSkybox;
uniform bool resetAttentuation;
uniform bool resetFirstHit;
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

vec3 getAttenuation(ivec3 coord)
{
    int index = 3 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride is 3, axis order is x y z
    if (currentBuffer)
    {
        return vec3(priorAttenuation1[index + 0], priorAttenuation1[index + 1], priorAttenuation1[index + 2]);
    }
    else
    {
        return vec3(priorAttenuation2[index + 0], priorAttenuation2[index + 1], priorAttenuation2[index + 2]);
    }
}

void setLightAccumulation(ivec3 coord, vec3 value)
{
    int index = 3 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride of 3, axis order is x y z

    accumulatedLight1[0 + index] = value.x;
    accumulatedLight1[1 + index] = value.y;
    accumulatedLight1[2 + index] = value.z;

    accumulatedLight2[0 + index] = value.x;
    accumulatedLight2[1 + index] = value.y;
    accumulatedLight2[2 + index] = value.z;
}

void setSkyBox(ivec3 coord, vec3 value)
{
    int index = 3 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride of 3, axis order is x y z

    if (!currentBuffer)
    {
        accumulatedLight1[0 + index] = accumulatedLight2[0 + index] + value.x;
        accumulatedLight1[1 + index] = accumulatedLight2[1 + index] + value.y;
        accumulatedLight1[2 + index] = accumulatedLight2[2 + index] + value.z;
    }
    else
    {
        accumulatedLight2[0 + index] = accumulatedLight1[0 + index] + value.x;
        accumulatedLight2[1 + index] = accumulatedLight1[1 + index] + value.y;
        accumulatedLight2[2 + index] = accumulatedLight1[2 + index] + value.z;
    }
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

float sunAngularSize = 5;//The angle of the sun in diameter
float sunSize = cos(sunAngularSize * 3.14159265 / 180.0);//0.99;
vec3 sunDir = normalize(vec3(1, -1, 1));
float sunBrightness = 2;

void main()
{
    ivec3 texelCoord = ivec3(gl_GlobalInvocationID.xyz);

    if (resetLight)
    {
        setLightAccumulation(texelCoord, vec3(0));
    }

    if (resetAttentuation)
    {
        setAttenuation(texelCoord, vec3(1));
    }

    if ((resetFirstHit && texelCoord.z == 0) || drawSkybox)
    {
        vec3 startPos = getRayPosition(texelCoord);
        vec3 rayDir = normalize(getRayDirection(texelCoord));

        if (resetFirstHit && texelCoord.z == 0)
        {
            setFirstHitPosition(texelCoord, startPos + rayDir * 100000);
            setFirstHitMaterial(texelCoord, vec3(-1, 0, 0)); // The skybox has a roughness of -1
            setFirstHitNormal(texelCoord, rayDir);
        }

        if (drawSkybox)
        {
            vec3 attenuation = getAttenuation(texelCoord);

            if (dot(rayDir, sunDir) > sunSize)
            {
                setSkyBox(texelCoord, sunBrightness / (6.28318530718 * (1 - sunSize)) * vec3(1, 1, 1) * attenuation);
            }
            else if (dot(rayDir, vec3(0, 0, 1)) > 0)
            {
                setSkyBox(texelCoord, 1 * vec3(40, 77, 222) / 255 * attenuation);
            }
            else
            {
                setSkyBox(texelCoord, 0.1 * vec3(61, 150, 11) / 255 * attenuation);
            }
        }
    }
}
