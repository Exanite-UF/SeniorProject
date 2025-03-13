#version 460 core

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

// Padded to 32 bytes long (alignment of 16)
struct MaterialDefinition
{
    vec3 emission;
    float padding0;
    vec3 albedo;
    float padding1;
    vec3 metallicAlbedo;
    float padding2;
    float roughness;
    float metallic;

    float padding3;
    float padding4;
};

uniform ivec3 cellCount;
uniform uint occupancyMapLayerCount;
uniform uint occupancyMapIndices[10]; // Assume that at most there are 10 possible mip map textures (This is a massive amount)
uniform uint materialStartIndices[3];

uniform ivec3 resolution; //(xSize, ySize, raysPerPixel)
uniform vec3 voxelWorldPosition;
uniform vec4 voxelWorldRotation; // This is a quaternion
uniform vec3 voxelWorldScale; // Size of a voxel

uniform uint materialMapSize;
uniform float random; // This is used to make non-deterministic randomness

uniform bool isFirstRay;
uniform uint worldNumber;

layout(std430, binding = 0) buffer RayPosition
{
    float rayPosition[];
};

vec3 getRayPosition(ivec3 coord)
{
    int index = 3 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride of 3, axis order is x y z
    return vec3(rayPosition[0 + index], rayPosition[1 + index], rayPosition[2 + index]);
}

layout(std430, binding = 1) buffer RayDirection
{
    float rayDirection[];
};

vec3 getRayDirection(ivec3 coord)
{
    int index = 3 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride of 3, axis order is x y

    return vec3(rayDirection[0 + index], rayDirection[1 + index], rayDirection[2 + index]);
}

layout(std430, binding = 2) buffer RayPositionOut
{
    float rayPositionOut[];
};

void setRayPosition(ivec3 coord, vec3 value)
{
    int index = 3 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride of 3, axis order is x y z
    rayPositionOut[0 + index] = value.x;
    rayPositionOut[1 + index] = value.y;
    rayPositionOut[2 + index] = value.z;
}

layout(std430, binding = 3) buffer RayDirectionOut
{
    float rayDirectionOut[];
};

void setRayDirection(ivec3 coord, vec3 value)
{
    int index = 3 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride of 3, axis order is x y z
    rayDirectionOut[0 + index] = value.x;
    rayDirectionOut[1 + index] = value.y;
    rayDirectionOut[2 + index] = value.z;
}

layout(std430, binding = 4) buffer OccupancyMap
{
    readonly uint occupancyMap[];
};

// coord is a cell coord
uint getOccupancyByte(ivec3 coord, int mipMapTexture)
{
    ivec3 tempRes = cellCount / (1 << (2 * mipMapTexture)); // get the resolution of the requested mipmap
    int index = (coord.x + tempRes.x * (coord.y + tempRes.y * coord.z)) + int(occupancyMapIndices[mipMapTexture]);
    int bufferIndex = index / 4; // Divide by 4, because glsl does not support single byte data types, so a 4 byte data type is being used
    int bufferOffset = (index & 3); // Modulus 4 done using a bitmask

    return (occupancyMap[bufferIndex] & (255 << (8 * bufferOffset))) >> (8 * bufferOffset);
}

layout(std430, binding = 5) buffer MaterialMap
{
    readonly uint materialMap[];
};

uint getMaterialIndex(ivec3 voxelPosition)
{
    ivec3 voxelCount = cellCount * 2;
    int i16Index = voxelPosition.x + voxelCount.x * (voxelPosition.y + voxelCount.y * voxelPosition.z);
    int i32Index = i16Index / 2;
    int bitsShifted = (i16Index & 1) * 16;

    return (materialMap[i32Index] & (0xffff << bitsShifted)) >> bitsShifted;
}

layout(std430, binding = 6) buffer HitMisc
{
    float hitMisc[];
};

float getHitWasHit(ivec3 coord)
{
    int index = 2 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // axis order is x y z
    return hitMisc[index + 0];
}

void setHitWasHit(ivec3 coord, bool value)
{
    int index = 2 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // axis order is x y z
    hitMisc[index + 0] = (value) ? 1.0 : 0.0;
}

float getHitDist(ivec3 coord)
{
    int index = 2 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // axis order is x y z
    return hitMisc[index + 1];
}

void setHitDist(ivec3 coord, float value)
{
    int index = 2 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // axis order is x y z
    hitMisc[index + 1] = value;
}

// Each entry is 32 bytes long (There are 12 bytes of padding)
layout(std430, binding = 7) buffer MaterialDefinitions
{
    restrict MaterialDefinition materialDefinitions[];
};

layout(std430, binding = 8) buffer AttenuationIn
{
    restrict float attenuationIn[];
};

vec3 getPriorAttenuation(ivec3 coord)
{
    int index = 3 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride is 3, axis order is x y z

    return vec3(attenuationIn[index + 0], attenuationIn[index + 1], attenuationIn[index + 2]);
}

layout(std430, binding = 9) buffer AccumulatedLightIn
{
    restrict float accumulatedLightIn[];
};

layout(std430, binding = 10) buffer AttenuationOut
{
    restrict float attenuationOut[];
};

void setAttenuation(ivec3 coord, vec3 value)
{
    int index = 3 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride is 3, axis order is x y z

    attenuationOut[0 + index] = value.x;
    attenuationOut[1 + index] = value.y;
    attenuationOut[2 + index] = value.z;
}

layout(std430, binding = 11) buffer AccumulatedLightOut
{
    restrict float accumulatedLightOut[];
};

void changeLightAccumulation(ivec3 coord, vec3 deltaValue)
{
    int index = 3 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride of 3, axis order is x y z
    accumulatedLightOut[0 + index] = accumulatedLightIn[0 + index] + deltaValue.x;
    accumulatedLightOut[1 + index] = accumulatedLightIn[1 + index] + deltaValue.y;
    accumulatedLightOut[2 + index] = accumulatedLightIn[2 + index] + deltaValue.z;
}

layout(std430, binding = 12) buffer FirstHitNormal
{
    writeonly float firstHitNormal[];
};

void setFirstHitNormal(ivec3 coord, vec3 value)
{
    int index = 3 * (coord.x + resolution.x * (coord.y)); // Stride of 3, axis order is x y z
    firstHitNormal[0 + index] = value.x;
    firstHitNormal[1 + index] = value.y;
    firstHitNormal[2 + index] = value.z;
}

layout(std430, binding = 13) buffer FirstHitPosition
{
    writeonly float firstHitPosition[];
};

void setFirstHitPosition(ivec3 coord, vec3 value)
{
    int index = 3 * (coord.x + resolution.x * (coord.y)); // Stride of 3, axis order is x y z
    firstHitPosition[0 + index] = value.x;
    firstHitPosition[1 + index] = value.y;
    firstHitPosition[2 + index] = value.z;
}

layout(std430, binding = 14) buffer FirstHitMaterial
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

layout(std430, binding = 15) buffer ShouldCastMask
{
    readonly uint showCastMask[];
};

bool shouldCast(ivec3 coord){
    int index = (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride of 1, axis order is x y z
    return (showCastMask[index] & (1 << (worldNumber % 32))) != 0;
}



struct RayHit
{
    bool wasHit;
    vec3 hitLocation;
    vec3 normal;
    float dist;
    int iterations;
    uint material;
    vec3 voxelHitLocation;
    bool isNearest;
};

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
    hit.isNearest = true;

    if (currentDepth <= 0)
    {
        hit.isNearest = false;
        return hit;
    }

    rayDir /= length(rayDir);

    vec3 aRayDir = 1 / abs(rayDir); // This is a constant that is used several times
    ivec3 sRayDir = ivec3(1.5 * rayDir / abs(rayDir)); // This is the sign of the ray direction (1.5 is for numerical stability)
    vec3 iRayDir = 1 / rayDir;

    ivec3 size = 2 * cellCount; // This is the size of the voxel volume

    vec3 rayStart = rayPos;

    // Put the ray at the surface of the cube
    float distToCube = rayboxintersect(rayStart, rayDir, vec3(0), vec3(size));
    rayPos += rayDir * max(0.0, distToCube - 0.001); // The -0.001 is for numerical stability when entering the volume (This is the aformentioned correction)

    // If the ray never entered the cube, then quit
    if (distToCube < 0)
    {
        hit.isNearest = false;
        return hit;
    }

    float depth = length(rayDir * voxelWorldScale * distToCube); // Find how far the ray has traveled from the start

    // If the start of the voxel volume is behind the currently closest thing, then there is not reason to continue
    if (depth > currentDepth)
    {
        hit.isNearest = false;
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
        if ((i > 0) && isOutsideVolume)
        {
            // No voxel was hit
            break;
        }

        int count = 0;
        // The <= is correct
        for (int i = 0; i <= occupancyMapLayerCount; i++)
        {
            ivec3 p2 = (p >> (2 * i)) & 1;
            uint k = ((1 << p2.x) << (p2.y << 1)) << (p2.z << 2); // This creates the mask that will extract the single bit that we want
            uint l = getOccupancyByte((p >> (1 + 2 * i)), i);
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
                hit.material = getMaterialIndex(p); // TODO: Set the material correctly
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

vec3 qtransform(vec4 q, vec3 v)
{
    return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + q.w * v);
}

RayHit rayCast(ivec3 texelCoord, vec3 startPos, vec3 rayDir, float currentDepth)
{
    vec3 rayPos = startPos;

    vec3 voxelWorldSize = 2. * cellCount;

    // Transform the ray position to voxel space
    rayPos -= voxelWorldPosition; // Find position relative to the voxel world
    rayPos = qtransform(vec4(-voxelWorldRotation.xyz, voxelWorldRotation.w), rayPos); // Inverse rotations lets us rotate from world space to voxel space
    rayPos /= voxelWorldScale; // Undo the scale now that we are aligned with voxel space
    rayPos += 0.5 * vec3(voxelWorldSize); // This moves the origin of the voxel world to its center

    // Transform the ray direction to voxel space
    rayDir = qtransform(vec4(-voxelWorldRotation.xyz, voxelWorldRotation.w), rayDir);
    rayDir /= voxelWorldScale;

    // Increment the ray forward slightly for numerical stability (This is corrected for in the intersection code)
    rayPos += rayDir * 0.001;

    // Find the intersection point of the ray cast
    RayHit hit = findIntersection(rayPos, rayDir, 200, currentDepth);

    hit.voxelHitLocation = hit.hitLocation; // Store the position of the intersection in voxel space

    // Transform the hit location to world space
    hit.hitLocation -= 0.5 * vec3(voxelWorldSize); // This moves the origin of the voxel world to its center
    hit.hitLocation *= voxelWorldScale; // Apply the scale of the voxel world
    hit.hitLocation = qtransform(voxelWorldRotation, hit.hitLocation); // Rotate back into world space
    hit.hitLocation += voxelWorldPosition; // Apply the voxel world position

    // Transform the hit normal from
    hit.normal *= voxelWorldScale;
    hit.normal = qtransform(voxelWorldRotation, hit.normal);

    hit.dist = length(startPos - hit.hitLocation);
    if (!hit.wasHit)
    {
        hit.dist = 1.0 / 0.0;
    }

    if (hit.dist > currentDepth || !hit.isNearest)
    {
        hit.isNearest = false;
        return hit;
    }

    hit.isNearest = true;

    // At this point was hit is true

    setHitDist(texelCoord, hit.dist);
    setHitWasHit(texelCoord, hit.wasHit);

    return hit;
}

float hash(float seed)
{
    return fract(sin((seed) * 12345.6789) * 43758.5453123);
}

vec2 randomVec2(float seed)
{
    return vec2(hash(seed), hash(seed + 1.0));
}

// For metals baseReflectivity is the metallicAlbedo
// For non-metals, it is a completely different property (I'm going to go with 1)
// So this parameter will be a linear interpolation based on the metallic parameter between 1 and the metallicAlbedo
vec3 fresnel(float dotOfLightAndHalfway, vec3 baseReflectivity)
{
    return baseReflectivity + (1 - baseReflectivity) * pow(abs(1 - dotOfLightAndHalfway), 5); // Schlick’s approximation
}

// This works in tangent space
// So the normal is (0, 0, 1)
float geometricBlockingGGX(float roughness, float dotOfLightAndNormal, float dotOfViewAndNormal)
{
    dotOfLightAndNormal = abs(dotOfLightAndNormal);
    dotOfViewAndNormal = abs(dotOfViewAndNormal);
    float numerator = 2 * dotOfLightAndNormal * dotOfViewAndNormal;
    float temp = roughness * roughness;
    float denominator1 = dotOfViewAndNormal * sqrt(temp + (1 - temp) * dotOfLightAndNormal * dotOfLightAndNormal);
    float denominator2 = dotOfLightAndNormal * sqrt(temp + (1 - temp) * dotOfViewAndNormal * dotOfViewAndNormal);
    return numerator / (denominator1 + denominator2);
}

// This works in tangent space
// So the normal is (0, 0, 1)
float microfacetDistributionGGX(float roughness, float dotOfNormalAndHalfway)
{
    float temp = roughness * roughness;
    float temp2 = dotOfNormalAndHalfway * dotOfNormalAndHalfway * (temp - 1) + 1;
    return temp / (3.1415926589 * temp * temp);
}

vec4 sampleGGX2(float roughness, vec2 rand, vec3 view, vec3 normal)
{
    float a = roughness * roughness;

    // -- Calculate theta and phi for our microfacet normal wm by
    // -- importance sampling the Ggx distribution of normals
    float theta = acos(sqrt((1.0f - rand.x) / ((a - 1.0f) * rand.x + 1.0f)));
    float phi = 6.28318530718 * rand.y;

    // -- Convert from spherical to Cartesian coordinates
    // wm is the microfacet direction
    vec3 microfacetDirection = vec3(
        cos(phi) * sin(theta),
        sin(phi) * sin(theta),
        cos(theta));

    vec3 temp = abs(normal.z) < 0.5 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0); // Pick a direction not parallel with the normal (Either pick up if it is definitely not facing up, or pick along the x axis if it is definitely pointing up)
    vec3 tangent = normalize(cross(temp, normal)); // Get a vector perpecdicular to the previous two
    vec3 bitangent = cross(normal, tangent); // Get a second vector that is perpendicular to the previous 2
    microfacetDirection = tangent * microfacetDirection.x + bitangent * microfacetDirection.y + normal * microfacetDirection.z;

    // -- Calculate light by reflecting view about wm
    vec3 light = reflect(view, microfacetDirection);

    // -- Ensure our sample is in the upper hemisphere
    if (dot(light, normal) < 0)
    {
        return vec4(light, 0);
    }
    else
    {
        vec3 halfway = normalize(view + light); // This is used by several things
        return vec4(light, 1 / (dot(microfacetDirection, normal) / abs(dot(view, microfacetDirection))));
    }
}

vec3 brdf(vec3 normal, vec3 view, vec3 light, MaterialDefinition voxelMaterial)
{
    vec3 halfway = normalize(-view + light); // This is used by several things

    float microfacetComponent = microfacetDistributionGGX(voxelMaterial.roughness, dot(normal, halfway)); // This is the component of the BRDF that accounts for the direction of microfacets (Based on the distribution of microfacet directions, what is the percent of light that reflects toward the camera)

    vec3 baseReflectivity = vec3(1 - voxelMaterial.metallic) + voxelMaterial.metallic * voxelMaterial.metallicAlbedo; // This is the metallic reflectivity (For non-metallic materials it is 1, for metallic materials is it the metallicAlbedo)

    vec3 fresnelComponent = fresnel(abs(dot(light, halfway)), baseReflectivity); // This component simulates the fresnel effect (only metallic materials have this)

    // This approximates how much light is blocked by microfacets, when looking from different directions
    float dotOfViewAndNormal = dot(view, normal);
    float dotOfLightAndNormal = dot(light, normal);
    float geometricComponent = geometricBlockingGGX(voxelMaterial.roughness, dot(light, normal), dot(view, normal)); // geometricBlocking(abs(dotOfViewAndNormal), abs(dotOfLightAndNormal), voxelMaterial.roughness); // Like how a mountain blocks the light in a valley

    // The effect of the metallicAlbedo is performed in the fresenl component
    // A non metallic material is assumed to not be affected by the fresnel effect
    // The fresnel component will color the reflected light, and will behave according to an approximation of the fresnel effect
    // Since albedo is about perfectly diffuse refection color, that would imply no fresnel effect
    // This does mean that there are two ways to get a colored mirror reflection. (one with and one without the fresnel effect)
    // It also means that the color of the metallic albedo will show off stronger at sharper angles
    // We add the metallic value to the albedo to prevent darkening. (this is a multiplier, so not doing this would just make metals black)
    vec3 albedo = (1 - voxelMaterial.metallic) * voxelMaterial.albedo + voxelMaterial.metallic;

    return microfacetComponent * fresnelComponent * geometricComponent * albedo / abs(4 * dotOfViewAndNormal * dotOfLightAndNormal);
}

vec3 brdf2(vec3 normal, vec3 view, vec3 light, MaterialDefinition voxelMaterial)
{

    vec3 halfway = normalize(-view + light); // This is used by several things

    // This component is part of the sampling distribution pdf, so it cancels out
    // float microfacetComponent = microfacetDistributionGGX(voxelMaterial.roughness, dot(normal, halfway));//This is the component of the BRDF that accounts for the direction of microfacets (Based on the distribution of microfacet directions, what is the percent of light that reflects toward the camera)

    vec3 baseReflectivity = vec3(1 - voxelMaterial.metallic) + voxelMaterial.metallic * voxelMaterial.metallicAlbedo; // This is the metallic reflectivity (For non-metallic materials it is 1, for metallic materials is it the metallicAlbedo)

    vec3 fresnelComponent = fresnel(abs(dot(light, halfway)), baseReflectivity); // This component simulates the fresnel effect (only metallic materials have this)

    // This approximates how much light is blocked by microfacets, when looking from different directions
    float dotOfViewAndNormal = dot(view, normal);
    float dotOfLightAndNormal = dot(light, normal);
    float geometricComponent = geometricBlockingGGX(voxelMaterial.roughness, dot(light, normal), dot(view, normal)); // geometricBlocking(abs(dotOfViewAndNormal), abs(dotOfLightAndNormal), voxelMaterial.roughness); // Like how a mountain blocks the light in a valley

    // The effect of the metallicAlbedo is performed in the fresenl component
    // A non metallic material is assumed to not be affected by the fresnel effect
    // The fresnel component will color the reflected light, and will behave according to an approximation of the fresnel effect
    // Since albedo is about perfectly diffuse refection color, that would imply no fresnel effect
    // This does mean that there are two ways to get a colored mirror reflection. (one with and one without the fresnel effect)
    // It also means that the color of the metallic albedo will show off stronger at sharper angles
    // We add the metallic value to the albedo to prevent darkening. (this is a multiplier, so not doing this would just make metals black)
    vec3 albedo = (1 - voxelMaterial.metallic) * voxelMaterial.albedo + voxelMaterial.metallic;

    return fresnelComponent * geometricComponent * albedo / abs(dotOfViewAndNormal);
}

// It is guaranteeed to be a hit
void BRDF(ivec3 texelCoord, RayHit hit, vec3 rayDirection, vec3 attentuation)
{
    float seed = random + float(texelCoord.x + resolution.x * (texelCoord.y + resolution.y * texelCoord.z)) / resolution.x / resolution.y / resolution.z; // texelCoord.x + texelCoord.y * 1.61803398875 + texelCoord.z * 3.1415926589;

    vec3 direction = rayDirection; // The direction the ray cast was in

    vec3 position = hit.hitLocation;

    // Load the hit normal
    // float dist = getHitDist(texelCoord); // Distance that the ray cast covered
    vec3 normal = hit.normal; // The normal direction of the hit

    // Find the uv coordinate for the texture
    // It is based on the hit location in voxel space
    // vec3 voxelPosition = hit.voxelHitLocation;
    // vec2 hitUV;
    // if (abs(normal.x) > 0)
    //{
    //    // yz
    //    hitUV = voxelPosition.yz;
    //}
    // else if (abs(normal.y) > 0)
    //{
    //    // xz
    //    hitUV = voxelPosition.xz;
    //}
    // else if (abs(normal.z) > 0)
    //{
    //    // xy
    //    hitUV = voxelPosition.xy;
    //}
    // vec2 uv = hitUV * sizes[material]; // We need to set the material textures to SL_REPEAT mode (this is the default).

    // Format the voxel material into a struct
    // Load the correct material values from the array of material textures
    MaterialDefinition voxelMaterial = materialDefinitions[hit.material]; // Get the material index of the hit, and map it to an actual material

    // Multiply in the texture values
    /*
    voxelMaterial.emission *= texture(sampler2d(materialTextures[materialID].emission), uv).xyz;
    voxelMaterial.albedo *= texture(sampler2d(materialTextures[materialID].albedo), uv).xyz;
    voxelMaterial.metallicAlbedo *= texture(sampler2d(materialTextures[materialID].metallicAlbedo), uv).xyz;
    vec4 rmTexture = texture(sampler2d(materialTextures[materialID].rmTexture), uv);
    voxelMaterial.roughness *= rmTexture.r;
    voxelMaterial.metallic *= rmTexture.g;
    */

    if (texelCoord.z == 0 && isFirstRay)
    {
        setFirstHitMaterial(texelCoord, vec3(voxelMaterial.roughness, 0, 0));
    }

    normal = normalize(normal);
    direction = normalize(direction);

    vec4 nextDirection = sampleGGX2(voxelMaterial.roughness, randomVec2(seed), direction, normal);
    vec3 brdfValue = brdf2(normal, direction, nextDirection.xyz, voxelMaterial) * nextDirection.w;

    // vec4 nextDirection = sampleGGX(voxelMaterial.roughness, randomVec2(seed), direction, normal);
    // vec3 brdfValue = dot(nextDirection.xyz, normal) * brdf(normal, direction, nextDirection.xyz, voxelMaterial) * nextDirection.w;

    vec3 receivedLight = voxelMaterial.emission * attentuation;

    // Set the output buffers
    setRayPosition(texelCoord, position); // Set where the ray should start from next
    setRayDirection(texelCoord, normalize(nextDirection.xyz)); // Set the direction the ray should start from next

    setAttenuation(texelCoord, attentuation * brdfValue); // The attenuation for the next bounce is the current attenuation times the brdf
    changeLightAccumulation(texelCoord, receivedLight); // Accumulate the light the has reached the camera
}

float sunSize = 0.99;
vec3 sunDir = normalize(vec3(1, -1, 1));
float sunBrightness = 5;

void attempt(ivec3 texelCoord)
{
    //if(!shouldCast(texelCoord)){
    //    return;
    //}
    float currentDepth = getHitDist(texelCoord);
    if(currentDepth < 0){
        return;
    }


    vec3 startPos = getRayPosition(texelCoord);
    vec3 rayDir = normalize(getRayDirection(texelCoord));
    
    
    RayHit hit = rayCast(texelCoord, startPos, rayDir, currentDepth);

    // If it is not the nearest, then it should do nothing
    // If it did not hit, then it should do nothing
    if (!hit.isNearest || !hit.wasHit)
    {
        return;
    }

    if (texelCoord.z == 0 && isFirstRay)
    {
        setFirstHitNormal(texelCoord, hit.normal);
        setFirstHitPosition(texelCoord, hit.hitLocation);
    }

    vec3 attentuation = getPriorAttenuation(texelCoord); // This is the accumulated attenuation
    BRDF(texelCoord, hit, rayDir, attentuation);
}

void main()
{
    ivec3 texelCoord = ivec3(gl_GlobalInvocationID.xyz);
    attempt(texelCoord);
    //setHitWasHit(texelCoord, false);
    //setHitDist(texelCoord, 1.0 / 0.0);
}
