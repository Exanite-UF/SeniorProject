#version 460 core

layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;

layout(binding = 0, r32ui) uniform coherent uimage3D voxelTexture;

layout(binding = 1, std140) buffer TriangleBuffer
{
    vec3 triangles[];
};

uniform int triCount;
uniform vec3 gridSize;
uniform vec3 minBounds;
uniform vec3 maxBounds;

bool intersectRayTriangle(vec3 rayOrigin, vec3 rayDir, vec3 v0, vec3 v1, vec3 v2, out float t)
{
    vec3 edge1 = v1 - v0;
    vec3 edge2 = v2 - v0;
    vec3 h = cross(rayDir, edge2);
    float a = dot(edge1, h);
    if (a > -1e-6 && a < 1e-6)
    {
        return false; // No Intersection
    }

    float f = 1.0 / a;
    vec3 s = rayOrigin - v0;
    float u = f * dot(s, h);
    if (u < 0.0 || u > 1.0)
    {
        return false; // No Intersection
    }

    vec3 q = cross(s, edge1);
    float v = f * dot(rayDir, q);
    if (v < 0.0 || u + v > 1.0)
    {
        return false; // No Intersection
    }

    t = f * dot(edge2, q);
    return t > 1e-6; // Intersection
}

void main()
{
    ivec3 voxelCoord = ivec3(gl_GlobalInvocationID.xyz);

    if (any(greaterThanEqual(voxelCoord, gridSize)))
        return;

    vec3 normalized = (vec3(voxelCoord) + 0.5) / vec3(gridSize);
    vec3 worldPos = mix(minBounds, maxBounds, normalized);

    // vec3 boundsSize = maxBounds - minBounds;
    // vec3 voxelSize = boundsSize / vec3(gridSize);
    // vec3 worldPos = minBounds + vec3(gl_GlobalInvocationID) * voxelSize + voxelSize * 0.5;

    // Ray Setup
    vec3 rayOrigin = worldPos + vec3(0.0, 0.0, (maxBounds.z - minBounds.z) / gridSize.z);
    vec3 rayDir = normalize(vec3(0.0, 0.0, -1.0));

    for (int i = 0; i < triCount; i++)
    {
        vec3 v0 = triangles[i * 3 + 0];
        vec3 v1 = triangles[i * 3 + 1];
        vec3 v2 = triangles[i * 3 + 2];

        float t;
        if (intersectRayTriangle(rayOrigin, rayDir, v0, v1, v2, t))
        {
            imageAtomicAdd(voxelTexture, voxelCoord, 1u);
            // imageAtomicAdd(voxelTexture, uvec3(voxelCoord), 1u);
            // imageStore(voxelTexture, ivec3(voxelCoord), uvec4(1));
        }
    }
}
