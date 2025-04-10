#version 460 core
#extension GL_EXT_shader_explicit_arithmetic_types_float16 : enable
#extension GL_NV_gpu_shader5 : enable


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
    readonly restrict float16_t accumulatedLightIn[];
};

layout(std430, binding = 3) buffer AccumulatedLightOut
{
    writeonly float16_t accumulatedLightOut[];
};

layout(std430, binding = 4) buffer AttenuationIn
{
    restrict float16_t attenuationIn[];
};

vec3 getPriorAttenuation(ivec3 coord)
{
    int index = 3 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride is 3, axis order is x y z

    return vec3(attenuationIn[index + 0], attenuationIn[index + 1], attenuationIn[index + 2]);
}

void changeLightAccumulation(ivec3 coord, vec3 deltaValue)
{
    int index = 3 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride of 3, axis order is x y z
    accumulatedLightOut[0 + index] = float16_t(accumulatedLightIn[0 + index] + deltaValue.x);
    accumulatedLightOut[1 + index] = float16_t(accumulatedLightIn[1 + index] + deltaValue.y);
    accumulatedLightOut[2 + index] = float16_t(accumulatedLightIn[2 + index] + deltaValue.z);
}

uniform bool shouldDrawSkybox;
uniform float sunAngularSize; // The angle of the sun in diameter
float sunSize = cos(sunAngularSize * 3.14159265 / 180.0 * 0.5);
uniform vec3 sunDir;
uniform float sunBrightnessMultiplier;
uniform float skyBrightnessMultiplier;

layout(binding = 0) uniform samplerCube skybox;

vec3 skyBox(vec3 rayDirection)
{
    float multiplier = skyBrightnessMultiplier;
    if (dot(normalize(sunDir), normalize(rayDirection)) > sunSize)
    {
        multiplier = sunBrightnessMultiplier;
    }
    return texture(skybox, vec3(-rayDirection.y, rayDirection.z, -rayDirection.x)).xyz * multiplier;
}

void main()
{
    ivec3 texelCoord = ivec3(gl_GlobalInvocationID.xyz);

    if (getRayDepth(texelCoord) >= 0)
    {
        setRayDepth(texelCoord, maxDepth);

        vec3 attenuation = getPriorAttenuation(texelCoord);
        if (shouldDrawSkybox)
        {
            changeLightAccumulation(texelCoord, skyBox(getRayDirection(texelCoord)) * attenuation);
        }
    }
    else
    {
        changeLightAccumulation(texelCoord, vec3(0));
    }
}
