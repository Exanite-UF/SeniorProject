#version 440

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(rgba8ui, binding = 0) uniform readonly uimage3D texture1;
layout(rgba8ui, binding = 1) uniform readonly uimage3D texture2;
layout(rgba8ui, binding = 2) uniform readonly uimage3D texture3;
// layout(rgba8ui, binding = 3) uniform readonly uimage3D texture4;
// layout(rgba8ui, binding = 4) uniform readonly uimage3D texture5;

layout(rgba32f, binding = 3) uniform readonly image3D rayPosition;
layout(rgba32f, binding = 4) uniform readonly image3D rayDirection;

layout(rgba32f, binding = 5) uniform image3D hitPosition;
layout(rgba32f, binding = 6) uniform image3D hitNormal;
layout(r16ui, binding = 7) uniform uimage3D hitMaterial;

uniform vec3 voxelWorldPosition;
uniform vec4 voxelWorldOrientation; // This is a quaternion
uniform vec3 voxelWorldScale; // Size of a voxel

vec3 qtransform(vec4 q, vec3 v)
{
    return v + 2.0 * cross(cross(v, q.xyz) + q.w * v, q.xyz);
}

struct RayHit
{
    bool wasHit;
    vec3 hitLocation;
    vec3 normal;
    float dist;
    int iterations;
    uint material;
};

// source: https://gamedev.stackexchange.com/questions/18436/most-efficient-aabb-vs-ray-collision-algorithms
float rayboxintersect(vec3 raypos, vec3 raydir, vec3 boxmin, vec3 boxmax)
{
    if (all(greaterThan(raypos, boxmin)) && all(lessThan(raypos, boxmax)))
    { // ray is inside the box
        return 0;
    }

    float t1 = (boxmin.x - raypos.x) / raydir.x;
    float t2 = (boxmax.x - raypos.x) / raydir.x;
    float t3 = (boxmin.y - raypos.y) / raydir.y;
    float t4 = (boxmax.y - raypos.y) / raydir.y;
    float t5 = (boxmin.z - raypos.z) / raydir.z;
    float t6 = (boxmax.z - raypos.z) / raydir.z;

    float tmin = max(max(min(t1, t2), min(t3, t4)), min(t5, t6));
    float tmax = min(min(max(t1, t2), max(t3, t4)), max(t5, t6));

    if (tmax < 0.0) // box on ray but behind ray origin
    {
        return -1;
    }

    if (tmin > tmax) // ray doesn't intersect box
    {
        return -1;
    }

    return tmin;
}

// Minimal aliasing (Fast)
RayHit findIntersection(vec3 rayPos, vec3 rayDir, int maxIterations, float currentDepth)
{
    RayHit hit;
    hit.material = 0;
    hit.wasHit = false;

    rayDir /= length(rayDir);

    vec3 aRayDir = 1 / abs(rayDir); // This is a constant that is used several times
    ivec3 sRayDir = ivec3(1.5 * rayDir / abs(rayDir)); // This is the sign of the ray direction (1.5 is for numerical stability)
    vec3 iRayDir = 1 / rayDir;

    ivec3 size = 2 * imageSize(texture1); // This is the size of the voxel volume

    vec3 rayStart = rayPos;

    // Put the ray at the surface of the cube
    float distToCube = rayboxintersect(rayStart, rayDir, vec3(0), vec3(size));
    rayPos += rayDir * max(0.0, distToCube - 0.001); // The -0.001 is for numerical stability when entering the volume

    // If the ray never entered the cube, then quit
    if (distToCube < 0)
    {
        return hit;
    }

    float depth = length(rayDir * voxelWorldScale * distToCube); // Find how far the ray has traveled from the start

    // If the start of the voxel volume is behind the currently closest thing, then there is not reason to continue
    if (depth > currentDepth)
    {
        return hit;
    }

    bool isOutside = false; // Used to make the image appear to be backface culled (It actually drastically decreases performance if rendered from inside the voxels)

    for (int i = 0; i < maxIterations; i++)
    {
        hit.iterations = i;
        ivec3 p = ivec3(floor(rayPos)); // voxel coordinate

        vec3 t = ceil(rayPos * sRayDir) * aRayDir - rayPos * iRayDir;
        t += vec3(lessThanEqual(t, vec3(0))) * aRayDir; // Numerical stability correction

        // Stop iterating if you leave the cube that all the voxels are in (1 unit of padding is provided to help with numerical stability)
        if (i > 1 && (any(greaterThan(p, ivec3(size - 1))) || any(lessThan(p, ivec3(0)))))
        {
            // No voxel was hit
            break;
        }

        ivec3 p2 = p & 1; // This lets us disambiguate between the 8 voxels in a cell
        uint k1 = ((1 << p2.x) << (p2.y << 1)) << (p2.z << 2); // This creates the mask that will extract the single bit that we want

        p2 = (p >> 2) & 1; // This lets us disambiguate between the 8 voxels in a cell of level 2
        uint k2 = ((1 << p2.x) << (p2.y << 1)) << (p2.z << 2); // This creates the mask that will extract the single bit that we want

        p2 = (p >> 4) & 1; // This lets us disambiguate between the 8 voxels in a cell of level 3
        uint k3 = ((1 << p2.x) << (p2.y << 1)) << (p2.z << 2); // This creates the mask that will extract the single bit that we want

        // p2 = (p >> 6) & 1; // This lets us disambiguate between the 8 voxels in a cell of level 4
        // uint k4 = ((1 << p2.x) << (p2.y << 1)) << (p2.z << 2); // This creates the mask that will extract the single bit that we want

        // p2 = (p >> 8) & 1; // This lets us disambiguate between the 8 voxels in a cell of level 5
        // uint k5 = ((1 << p2.x) << (p2.y << 1)) << (p2.z << 2); // This creates the mask that will extract the single bit that we want

        uvec4 l1 = imageLoad(texture1, (p >> 1));
        uvec4 l2 = imageLoad(texture2, (p >> 3));
        uvec4 l3 = imageLoad(texture3, (p >> 5));
        // uvec4 l4 = imageLoad(texture4, (p >> 7));
        // uvec4 l5 = imageLoad(texture5, (p >> 9));

        uint level1 = l1.a; // This is the cell from the image (Warning the upper 24 bits are garbage and should be ignored)

        uint level2 = l2.a; // cell for level 2

        uint level3 = l3.a; // cell for level 3

        // uint level4 = l4.a; // cell for level 4

        // uint level5 = l5.a; // cell for level 4

        // This is the number of mip map levels at which no voxels are found
        // int count = int(level5 == 0) + int((level5 & k5) == 0) + int(level4 == 0) + int((level4 & k4) == 0) + int(level3 == 0) + int((level3 & k3) == 0) + int(level2 == 0) + int((level2 & k2) == 0) + int(level1 == 0) + int((level1 & k1) == 0);
        int count = int(level3 == 0) + int((level3 & k3) == 0) + int(level2 == 0) + int((level2 & k2) == 0) + int(level1 == 0) + int((level1 & k1) == 0);

        if (count <= 0)
        {
            // This means that there was a hit
            if (i > 0 && isOutside)
            { // Don't intersect with the first voxel
                hit.wasHit = true;
                uvec3 c1 = uvec3(greaterThan(l1.rgb & k1, uvec3(0))); // uvec3((l1.r & k1) > 0 ?  1 : 0, (l1.g & k1) > 0 ? 1 : 0, (l1.b & k1) > 0 ? 1 : 0);
                uvec3 c2 = uvec3(greaterThan(l2.rgb & k2, uvec3(0))); // uvec3(l2.r & k2 > 0, l2.g & k2 > 0, l2.b & k2 > 0);
                uvec3 c3 = uvec3(greaterThan(l3.rgb & k3, uvec3(0))); // uvec3(l3.r & k3 > 0, l3.g & k3 > 0, l3.b & k3 > 0);
                hit.material = c1.r + (1 << 1) * c1.g + (1 << 2) * c1.b + (1 << 3) * c2.r + (1 << 4) * c2.g + (1 << 5) * c2.b + (1 << 6) * c3.r + (1 << 7) * c3.g + (1 << 8) * c3.b;
                break;
            }
        }
        else
        {
            isOutside = true;
            // This calculates how far a mip map level should jump
            t += mod(floor(-sRayDir * rayPos), (1 << (count - 1))) * aRayDir; // This uses the number of mip maps where there are no voxels, to determine how far to jump
        }

        // Find which jump amount to use next
        float minT = min(min(t.x, t.y), t.z);
        hit.normal = -sRayDir * ivec3(minT == t.x, minT == t.y, minT == t.z); // Set the normal

        rayPos += rayDir * (minT)-hit.normal * 0.001; // 0.001 is for numerical stability (yes it causes a small aliasing artifact)
    }

    hit.hitLocation = rayPos;
    hit.dist = length(rayStart - hit.hitLocation);

    return hit;
}

void main()
{
    ivec3 texelCoord = ivec3(gl_GlobalInvocationID.xyz);

    vec3 rayPos = imageLoad(rayPosition, texelCoord).xyz;
    vec3 rayDir = imageLoad(rayDirection, texelCoord).xyz;

    vec3 rayStart = rayPos;

    float currentDepth = imageLoad(hitNormal, texelCoord).w;

    vec3 voxelWorldSize = 2. * imageSize(texture1) * voxelWorldScale;

    rayPos -= voxelWorldPosition;// - 0.5 * vec3(voxelWorldSize);
    rayPos = qtransform(vec4(-voxelWorldOrientation.xyz, voxelWorldOrientation.w), rayPos);
    rayPos += 0.5 * vec3(voxelWorldSize);
    rayPos /= voxelWorldScale;

    rayDir = qtransform(vec4(-voxelWorldOrientation.xyz, voxelWorldOrientation.w), rayDir);
    rayDir /= voxelWorldScale;

    RayHit hit = findIntersection(rayPos, rayDir, 200, currentDepth);
    hit.hitLocation *= voxelWorldScale;
    hit.hitLocation = qtransform(voxelWorldOrientation, hit.hitLocation);

    hit.normal = qtransform(voxelWorldOrientation, hit.normal);

    hit.dist = length(rayDir * voxelWorldScale * hit.dist);//length(hit.hitLocation - rayStart);

    if (hit.wasHit && hit.dist < currentDepth)
    {
        imageStore(hitPosition, texelCoord, vec4(hit.hitLocation, hit.wasHit));
        imageStore(hitNormal, texelCoord, vec4(hit.normal, hit.dist));
        imageStore(hitMaterial, texelCoord, uvec4(hit.material));
    }
}
