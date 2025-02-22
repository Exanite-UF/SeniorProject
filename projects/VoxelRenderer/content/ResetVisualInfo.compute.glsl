#version 440

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(std430, binding = 0) buffer PriorAttenuation
{
    float priorAttenuation[];
};

layout(std430, binding = 1) buffer AccumulatedLight
{
    float accumulatedLight[];
};

uniform ivec3 resolution; //(xSize, ySize, raysPerPixel)

void setAttenuation(ivec3 coord, vec3 value)
{
    int index = 3 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride is 3, axis order is x y z

    priorAttenuation[0 + index] = value.x;
    priorAttenuation[1 + index] = value.y;
    priorAttenuation[2 + index] = value.z;
}



void setLightAccumulation(ivec3 coord, vec3 value)
{
    int index = 3 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride of 3, axis order is x y z
    accumulatedLight[0 + index] = value.x;
    accumulatedLight[1 + index] = value.y;
    accumulatedLight[2 + index] = value.z;
}

void multiplyLightAccumulation(ivec3 coord, vec3 value)
{
    int index = 3 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride of 3, axis order is x y z
    accumulatedLight[0 + index] *= value.x;
    accumulatedLight[1 + index] *= value.y;
    accumulatedLight[2 + index] *= value.z;
}


// layout(rgba32f, binding = 0) uniform writeonly image3D hitPosition;
// layout(rgba32f, binding = 1) uniform writeonly image3D hitNormal;
// layout(r16ui, binding = 2) uniform writeonly uimage3D hitMaterial;

void main()
{
    ivec3 texelCoord = ivec3(gl_GlobalInvocationID.xyz);

    setAttenuation(texelCoord, vec3(1));
    setLightAccumulation(texelCoord, vec3(0));
    
    
    // imageStore(hitPosition, texelCoord, vec4(0));
    // imageStore(hitNormal, texelCoord, vec4(vec3(0), 1.0 / 0.0));
    // imageStore(hitMaterial, texelCoord, uvec4(0));
}
