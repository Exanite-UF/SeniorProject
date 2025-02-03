#version 440

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;


layout(rgba32f, binding = 0) uniform writeonly image3D hitPosition;
layout(rgba32f, binding = 1) uniform writeonly image3D hitNormal;
layout(r16ui, binding = 2) uniform writeonly uimage3D hitMaterial;


void main()
{
    ivec3 texelCoord = ivec3(gl_GlobalInvocationID.xyz);
    
    imageStore(hitPosition, texelCoord, vec4(0));
    imageStore(hitNormal, texelCoord, vec4(vec3(0), 1.0 / 0.0));
    imageStore(hitMaterial, texelCoord, vec4(0));
}
