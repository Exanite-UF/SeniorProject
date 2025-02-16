#version 440

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(std430, binding = 0) buffer RayPosition
{
    float rayPosition[];
};
layout(std430, binding = 1) buffer RayDirection
{
    float rayDirection[];
};

layout(std430, binding = 2) buffer OccupancyMap
{
    uint occupancyMap[];
};

layout(std430, binding = 3) buffer MaterialMap
{
    uint materialMap[];
};

// layout(rgba8ui, binding = 0) uniform readonly uimage3D texture1;
// layout(rgba8ui, binding = 1) uniform readonly uimage3D texture2;
// layout(rgba8ui, binding = 2) uniform readonly uimage3D texture3;
// layout(rgba8ui, binding = 3) uniform readonly uimage3D texture4;
// layout(rgba8ui, binding = 4) uniform readonly uimage3D texture5;

layout(rgba32f, binding = 5) uniform image3D hitPosition;
layout(rgba32f, binding = 6) uniform image3D hitNormal;
layout(r16ui, binding = 7) uniform uimage3D hitMaterial;

uniform ivec3 voxelResolution; //(xSize, ySize, zSize) size of the texture (not the size of the voxel world)
uniform uint mipMapTextureCount;
uniform uint mipMapStartIndices[10]; // Assume that at most there are 10 possible mip map textures (This is a massive amount)
uniform uint materialStartIndices[3];

uniform ivec3 resolution; //(xSize, ySize, raysPerPixel)
uniform vec3 voxelWorldPosition;
uniform vec4 voxelWorldRotation; // This is a quaternion
uniform vec3 voxelWorldScale; // Size of a voxel

// coord is a cell coord
uint getByte(ivec3 coord, int mipMapTexture)
{
    ivec3 tempRes = voxelResolution / (1 << (2 * mipMapTexture)); // get the resolution of the requested mipmap
    int index = (coord.x + tempRes.x * (coord.y + tempRes.y * coord.z)) + int(mipMapStartIndices[mipMapTexture]);
    int bufferIndex = index / 4; // Divide by 4, because glsl does not support single byte data types, so a 4 byte data type is being used
    int bufferOffset = (index & 3); // Modulus 4 done using a bitmask

    return (occupancyMap[bufferIndex] & (255 << (8 * bufferOffset))) >> (8 * bufferOffset);
}

// Coord is a voxel coord
uint getMaterial(ivec3 coord)
{
    uint result = 0;
    uint counter = 0;
    for (int level = 0; level < 3; level++)
    {
        ivec3 tempCoord = coord / (1 << (2 * level));
        ivec3 cellCoord = tempCoord / 2;

        ivec3 p2 = tempCoord & 1; // Modulus 2 the voxel coordinate using a bitmask
        uint k = ((1 << p2.x) << (p2.y << 1)) << (p2.z << 2); // make the bitmask that corresponds to the correct bit in the byte that we want

        ivec3 tempRes = voxelResolution / (1 << (2 * level)); // get the resolution of the requested level
        int index = cellCoord.x + tempRes.x * (cellCoord.y + tempRes.y * cellCoord.z); // + int(materialStartIndices[level]);

        // 4 bits are used for a single material and these bits are spread across 4 bytes, so the index of the cell is the index of the uint

        // These grab the 4 bits we want from the uint
        uint temp = materialMap[index];
        result |= int((temp & (k << 0)) != 0) << counter;
        result |= int((temp & (k << 8)) != 0) << (counter + 1);
        result |= int((temp & (k << 16)) != 0) << (counter + 2);
        result |= int((temp & (k << 24)) != 0) << (counter + 3);

        counter += 4;
    }

    return result;
}

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
    hit.iterations = 0;
    hit.material = 0;
    hit.wasHit = false;

    rayDir /= length(rayDir);

    vec3 aRayDir = 1 / abs(rayDir); // This is a constant that is used several times
    ivec3 sRayDir = ivec3(1.5 * rayDir / abs(rayDir)); // This is the sign of the ray direction (1.5 is for numerical stability)
    vec3 iRayDir = 1 / rayDir;

    ivec3 size = 2 * voxelResolution; // This is the size of the voxel volume

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

    bool isOutside = true; // Used to make the image appear to be backface culled (It actually drastically decreases performance if rendered from inside the voxels)

    for (int i = 0; i < maxIterations; i++)
    {
        hit.iterations = i;
        ivec3 p = ivec3(floor(rayPos)); // voxel coordinate

        vec3 t = ceil(rayPos * sRayDir) * aRayDir - rayPos * iRayDir;
        t += vec3(lessThanEqual(t, vec3(0))) * aRayDir; // Numerical stability correction

        // Stop iterating if you leave the cube that all the voxels are in (1 unit of padding is provided to help with numerical stability)
        bool isOutsideVolume = (any(greaterThan(p, ivec3(size - 1))) || any(lessThan(p, ivec3(0))));
        if ((i > 1) && isOutsideVolume)
        {
            // No voxel was hit
            break;
        }

        int count = 0;
        // The <= is correct
        for (int i = 0; i <= mipMapTextureCount; i++)
        {
            ivec3 p2 = (p >> (2 * i)) & 1;
            uint k = ((1 << p2.x) << (p2.y << 1)) << (p2.z << 2); // This creates the mask that will extract the single bit that we want
            uint l = getByte((p >> (1 + 2 * i)), i);
            count += int((l & k) == 0) + int(l == 0);
        }

        if (count <= 0)
        {

            // This means that there was a hit
            if (i > 0 && isOutside && !isOutsideVolume)
            { // Don't intersect with the first voxel
                hit.wasHit = true;
                // TODO: Reimplement Materials to use a buffer instead of an image
                // At the moment materials are not being store at all

                // uvec3 c1 = uvec3(greaterThan(l1.rgb & k1, uvec3(0))); // uvec3((l1.r & k1) > 0 ?  1 : 0, (l1.g & k1) > 0 ? 1 : 0, (l1.b & k1) > 0 ? 1 : 0);
                // uvec3 c2 = uvec3(greaterThan(l2.rgb & k2, uvec3(0))); // uvec3(l2.r & k2 > 0, l2.g & k2 > 0, l2.b & k2 > 0);
                // uvec3 c3 = uvec3(greaterThan(l3.rgb & k3, uvec3(0))); // uvec3(l3.r & k3 > 0, l3.g & k3 > 0, l3.b & k3 > 0);
                // hit.material = c1.r + (1 << 1) * c1.g + (1 << 2) * c1.b + (1 << 3) * c2.r + (1 << 4) * c2.g + (1 << 5) * c2.b + (1 << 6) * c3.r + (1 << 7) * c3.g + (1 << 8) * c3.b;
                hit.material = getMaterial(p); // TODO: Set the material correctly
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

vec3 getPos(ivec3 coord)
{
    int index = 3 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride of 3, axis order is x y z
    return vec3(rayPosition[0 + index], rayPosition[1 + index], rayPosition[2 + index]);
}

vec3 getDir(ivec3 coord)
{
    int index = 3 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride of 3, axis order is x y z
    return vec3(rayDirection[0 + index], rayDirection[1 + index], rayDirection[2 + index]);
}

void main()
{
    ivec3 texelCoord = ivec3(gl_GlobalInvocationID.xyz);

    vec3 rayPos = getPos(texelCoord); // imageLoad(rayPosition, texelCoord).xyz;
    vec3 rayDir = getDir(texelCoord); // imageLoad(rayDirection, texelCoord).xyz;

    vec3 rayStart = rayPos;

    float currentDepth = imageLoad(hitNormal, texelCoord).w;

    vec3 voxelWorldSize = 2. * voxelResolution;//The size of the voxel world in voxels
    
    rayPos -= voxelWorldPosition; // - 0.5 * vec3(voxelWorldSize);
    rayPos = qtransform(vec4(-voxelWorldRotation.xyz, voxelWorldRotation.w), rayPos);
    rayPos /= voxelWorldScale;
    rayPos += 0.5 * vec3(voxelWorldSize);

    rayDir = qtransform(vec4(-voxelWorldRotation.xyz, voxelWorldRotation.w), rayDir);
    rayDir /= voxelWorldScale;

    rayPos += rayDir * 0.001;

    RayHit hit = findIntersection(rayPos, rayDir, 200, currentDepth);

    hit.hitLocation -= 0.5 * vec3(voxelWorldSize);
    hit.hitLocation *= voxelWorldScale;
    hit.hitLocation = qtransform(voxelWorldRotation, hit.hitLocation);
    hit.hitLocation += voxelWorldPosition;

    hit.normal = qtransform(voxelWorldRotation, hit.normal);

    hit.dist = length(rayDir * voxelWorldScale * hit.dist); // length(hit.hitLocation - rayStart);

    if (hit.wasHit && hit.dist < currentDepth)
    {
        imageStore(hitPosition, texelCoord, vec4(hit.hitLocation, hit.wasHit)); // Record the world space position of the hit surface
        imageStore(hitNormal, texelCoord, vec4(hit.normal, hit.dist)); // Record the world space normal direction of the hit surface
        imageStore(hitMaterial, texelCoord, uvec4(hit.material));
    }

    // imageStore(hitMaterial, texelCoord, uvec4(hit.iterations));//Record the number of iterations into the material texture
}
