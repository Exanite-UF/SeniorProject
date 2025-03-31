#version 460 core

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

uniform ivec3 resolution; //(xSize, ySize, raysPerPixel)


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


void setAttenuation(ivec3 coord, vec3 value)
{
    int index = 3 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride is 3, axis order is x y z

    priorAttenuation1[0 + index] = value.x;
    priorAttenuation1[1 + index] = value.y;
    priorAttenuation1[2 + index] = value.z;
}

vec3 getAttenuation(ivec3 coord)
{
    int index = 3 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride is 3, axis order is x y z
    return vec3(priorAttenuation1[index + 0], priorAttenuation1[index + 1], priorAttenuation1[index + 2]);
}

void setLightAccumulation(ivec3 coord, vec3 value)
{
    int index = 3 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride of 3, axis order is x y z

    accumulatedLight1[0 + index] = value.x;
    accumulatedLight1[1 + index] = value.y;
    accumulatedLight1[2 + index] = value.z;
}

void setSkyBox(ivec3 coord, vec3 value)
{
    int index = 3 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride of 3, axis order is x y z

    accumulatedLight1[0 + index] += value.x;
    accumulatedLight1[1 + index] += value.y;
    accumulatedLight1[2 + index] += value.z;
}


float sunAngularSize = 20; // The angle of the sun in diameter
float sunSize = cos(sunAngularSize * 3.14159265 / 180.0);
vec3 sunDir = normalize(vec3(1, -1, 1));
float sunBrightness = 5;

void main()
{
    ivec3 texelCoord = ivec3(gl_GlobalInvocationID.xyz);

    setAttenuation(texelCoord, vec3(1));
    setLightAccumulation(texelCoord, vec3(0));
}
