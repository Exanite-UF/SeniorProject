#version 460 core
#extension GL_EXT_shader_explicit_arithmetic_types_float16 : enable
#extension GL_NV_gpu_shader5 : enable
#extension GL_ARB_bindless_texture : require
#extension GL_ARB_gpu_shader_int64 : enable

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

vec4 safeVec4(vec4 v, vec4 fallback)
{
    return vec4(
        isnan(v.r) || isinf(v.r) ? fallback.r : v.r,
        isnan(v.g) || isinf(v.g) ? fallback.g : v.g,
        isnan(v.b) || isinf(v.b) ? fallback.b : v.b,
        isnan(v.a) || isinf(v.a) ? fallback.a : v.a);
}

// Padded to 32 bytes long (alignment of 16)
struct MaterialDefinition
{
    vec3 emission;
    float textureScaleX;
    vec3 albedo;
    float textureScaleY;
    vec3 metallicAlbedo;
    float padding;
    float roughness;
    float metallic;

    uint64_t albedoTextureID;
    uint64_t roughnessTextureID;
    uint64_t emissionTextureID;
};

uniform vec3 pastCameraPosition;
uniform vec4 pastCameraRotation; // quaternion
uniform float pastCameraFovTan;

uniform vec3 pastVoxelWorldPosition;
uniform vec4 pastVoxelWorldRotation; // quaternion
uniform vec3 pastVoxelWorldScale;

uniform ivec3 cellCount;
uniform uint occupancyMapLayerCount;
uniform uint occupancyMapIndices[10]; // Assume that at most there are 10 possible mip map textures (This is a massive amount)
uniform uint materialStartIndices[3];

uniform ivec3 resolution; //(xSize, ySize, 1)
uniform vec3 voxelWorldPosition;
uniform vec4 voxelWorldRotation; // This is a quaternion
uniform vec3 voxelWorldScale; // Size of a voxel

uniform uint materialMapSize;
uniform float random; // This is used to make non-deterministic randomness

uniform float sunAngularSize; // The angle of the sun in diameter
uniform vec3 sunDirection;

uniform int firstMipMapLevel;
uniform int maxIterations;

layout(std430, binding = 0) buffer RayPosition
{
    float rayPosition[];
};

layout(std430, binding = 1) buffer RayPositionOut
{
    float rayPositionOut[];
};

vec3 getRayPosition(ivec3 coord)
{
    int index = 3 * (coord.x + resolution.x * coord.y); // Stride of 3, axis order is x y
    return vec3(rayPosition[0 + index], rayPosition[1 + index], rayPosition[2 + index]);
}

void setRayPosition(ivec3 coord, vec3 value)
{
    int index = 3 * (coord.x + resolution.x * coord.y); // Stride of 3, axis order is x y
    rayPositionOut[0 + index] = value.x;
    rayPositionOut[1 + index] = value.y;
    rayPositionOut[2 + index] = value.z;
}

layout(std430, binding = 2) buffer RayDirection
{
    float16_t rayDirection[];
};

layout(std430, binding = 3) buffer RayDirectionOut
{
    float16_t rayDirectionOut[];
};

vec3 getRayDirection(ivec3 coord)
{
    int index = 3 * (coord.x + resolution.x * coord.y); // Stride of 3, axis order is x y

    return vec3(rayDirection[0 + index], rayDirection[1 + index], rayDirection[2 + index]);
}

void setRayDirection(ivec3 coord, vec3 value)
{
    int index = 3 * (coord.x + resolution.x * coord.y); // Stride of 3, axis order is x y z
    rayDirectionOut[0 + index] = float16_t(value.x);
    rayDirectionOut[1 + index] = float16_t(value.y);
    rayDirectionOut[2 + index] = float16_t(value.z);
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

// Each entry is 32 bytes long (There are 12 bytes of padding)
layout(std430, binding = 6) buffer MaterialDefinitions
{
    restrict MaterialDefinition materialDefinitions[];
};

// Used for depth tests
layout(std430, binding = 7) buffer RayMisc
{
    float rayMisc[];
};

float getRayDepth(ivec3 coord)
{
    int index = 1 * (coord.x + resolution.x * coord.y); // axis order is x y
    return rayMisc[index + 0];
}

void setRayDepth(ivec3 coord, float value)
{
    int index = 1 * (coord.x + resolution.x * coord.y); // axis order is x y
    rayMisc[index + 0] = value;
}

layout(std430, binding = 8) buffer FirstHitNormal
{
    writeonly float firstHitNormal[];
};

void setFirstHitNormal(ivec3 coord, vec3 value)
{
    int index = 4 * (coord.x + resolution.x * (coord.y)); // Stride of 3, axis order is x y z
    firstHitNormal[0 + index] = value.x;
    firstHitNormal[1 + index] = value.y;
    firstHitNormal[2 + index] = value.z;
}

layout(std430, binding = 9) buffer FirstHitPosition
{
    writeonly float firstHitPosition[];
};

void setFirstHitPosition(ivec3 coord, vec3 value)
{
    int index = 4 * (coord.x + resolution.x * (coord.y)); // Stride of 3, axis order is x y z
    firstHitPosition[0 + index] = value.x;
    firstHitPosition[1 + index] = value.y;
    firstHitPosition[2 + index] = value.z;
}

uniform bool whichDepth;
void setFirstHitOldDepth(ivec3 coord, float value)
{
    int index = 4 * (coord.x + resolution.x * (coord.y)); // Stride of 4, axis order is x y z
    if (whichDepth)
    {
        firstHitNormal[3 + index] = value;
    }
    else
    {
        firstHitPosition[3 + index] = value;
    }
}

layout(std430, binding = 10) buffer FirstHitMaterialUV
{
    writeonly float16_t firstHitMaterialUV[];
};

void setFirstHitMaterialUV(ivec3 coord, vec2 value)
{
    int index = 2 * (coord.x + resolution.x * (coord.y)); // Stride of 1, axis order is x y
    firstHitMaterialUV[index + 0] = float16_t(value.x);
    firstHitMaterialUV[index + 1] = float16_t(value.y);
}

layout(std430, binding = 11) buffer FirstHitMaterial
{
    writeonly int firstHitMaterial[];
};

void setFirstHitMaterial(ivec3 coord, int value)
{
    int index = 1 * (coord.x + resolution.x * (coord.y)); // Stride of 1, axis order is x y
    firstHitMaterial[index] = value;
}

layout(std430, binding = 12) buffer SecondaryDirection
{
    writeonly float16_t secondaryDirection[];
};

void setSecondaryDirection(ivec3 coord, vec4 value)
{
    int index = 4 * (coord.x + resolution.x * (coord.y)); // Stride of 1, axis order is x y
    secondaryDirection[index + 0] = float16_t(value.x);
    secondaryDirection[index + 1] = float16_t(value.y);
    secondaryDirection[index + 2] = float16_t(value.z);
    secondaryDirection[index + 3] = float16_t(value.w);
}

uniform bool whichAccumulatingVector;
layout(std430, binding = 13) buffer AccumulatingMotionVectors
{
    float16_t accumulatingMotionVectors[];
};

void changeAccumulatingMotionVectors(ivec3 coord, vec2 value)
{
    int index = 4 * (coord.x + resolution.x * (coord.y)); // Stride of 1, axis order is x y
    if (whichAccumulatingVector)
    {

        float temp = accumulatingMotionVectors[index + 2] + (value.x);
        if (isnan(temp) || isinf(temp))
        {
            temp = 0;
        }
        accumulatingMotionVectors[index + 0] = float16_t(temp);

        temp = accumulatingMotionVectors[index + 3] + (value.y);
        if (isnan(temp) || isinf(temp))
        {
            temp = 0;
        }
        accumulatingMotionVectors[index + 1] = float16_t(temp);
    }
    else
    {
        float temp = accumulatingMotionVectors[index + 0] + (value.x);
        if (isnan(temp) || isinf(temp))
        {
            temp = 0;
        }
        accumulatingMotionVectors[index + 2] = float16_t(temp);

        temp = accumulatingMotionVectors[index + 1] + (value.y);
        if (isnan(temp) || isinf(temp))
        {
            temp = 0;
        }
        accumulatingMotionVectors[index + 3] = float16_t(temp);
    }
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
    vec2 uv; // voxel hit position relevant to finding texture coords
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
    rayPos += rayDir * max(0.0, distToCube - 0.1); // The -0.001 is for numerical stability when entering the volume (This is the aformentioned correction)

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
    bool hasEntered = false;
    // hit.wasHit = true;
    // hit.hitLocation = rayPos;
    // hit.dist = length(rayStart - hit.hitLocation);
    // return hit;

    for (int i = 0; i < maxIterations; i++)
    {
        hit.iterations = i;
        ivec3 p = ivec3(floor(rayPos)); // voxel coordinate

        vec3 t = ceil(rayPos * sRayDir) * aRayDir - rayPos * iRayDir;
        t += vec3(lessThanEqual(t, vec3(0))) * aRayDir; // Numerical stability correction

        // Stop iterating if you leave the cube that all the voxels are in (1 unit of padding is provided to help with numerical stability)
        bool isOutsideVolume = (any(greaterThan(p, ivec3(size - 1))) || any(lessThan(p, ivec3(0))));
        if (!isOutsideVolume)
        {
            hasEntered = true;
        }

        if ((i > 0) && isOutsideVolume && hasEntered)
        {
            // No voxel was hit
            break;
        }

        int count = 0;
        // The <= is correct
        // iterate by 1 until it enters the voxel volume
        if (isOutsideVolume)
        {
            count = 1;
        }
        else
        {
            for (int i = firstMipMapLevel / 2; i <= occupancyMapLayerCount; i++)
            {
                ivec3 p2 = (p >> (2 * i)) & 1;
                uint k = ((1 << p2.x) << (p2.y << 1)) << (p2.z << 2); // This creates the mask that will extract the single bit that we want
                uint l = getOccupancyByte((p >> (1 + 2 * i)), i);
                count += int((l & k) == 0) + int(l == 0);
            }

        }

        if (count <= firstMipMapLevel % 2)
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
            count -= firstMipMapLevel % 2;
            count += firstMipMapLevel;
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
    rayPos += rayDir * 0.01 * 0;

    // Find the intersection point of the ray cast
    RayHit hit = findIntersection(rayPos, rayDir, maxIterations, currentDepth);

    hit.voxelHitLocation = hit.hitLocation; // Store the position of the intersection in voxel space

    // Transform the hit location to world space
    hit.hitLocation -= 0.5 * vec3(voxelWorldSize); // This moves the origin of the voxel world to its center
    hit.hitLocation *= voxelWorldScale; // Apply the scale of the voxel world
    hit.hitLocation = qtransform(voxelWorldRotation, hit.hitLocation); // Rotate back into world space
    hit.hitLocation += voxelWorldPosition; // Apply the voxel world position

    // Calculate the hit uv
    if (abs(dot(hit.normal, vec3(1, 0, 0))) > 0.1)
    {
        hit.uv = hit.voxelHitLocation.yz;
    }
    else if (abs(dot(hit.normal, vec3(0, 1, 0))) > 0.1)
    {
        hit.uv = hit.voxelHitLocation.xz;
    }
    else
    {
        hit.uv = hit.voxelHitLocation.xy;
    }

    // Transform the hit normal from voxel space to world space
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

    // setRayDepth(texelCoord, hit.dist);
    // setHitWasHit(texelCoord, hit.wasHit);

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

float rgbToHue(vec3 rgb)
{
    float minC = min(rgb.r, min(rgb.g, rgb.b));
    float maxC = max(rgb.r, max(rgb.g, rgb.b));
    float delta = maxC - minC;

    float hue = 0.0;

    if (delta > 0.0)
    {
        if (maxC == rgb.r)
        {
            hue = mod((rgb.g - rgb.b) / delta, 6.0);
        }
        else if (maxC == rgb.g)
        {
            hue = ((rgb.b - rgb.r) / delta) + 2.0;
        }
        else
        { // maxC == rgb.b
            hue = ((rgb.r - rgb.g) / delta) + 4.0;
        }
        hue *= 60.0;
        if (hue < 0.0)
            hue += 360.0;
    }

    return hue / 360.;
}

void main()
{
    ivec3 texelCoord = ivec3(gl_GlobalInvocationID.xyz);

    if (texelCoord.x < 0)
    {
        return;
    }

    float currentDepth = getRayDepth(texelCoord);

    // Early quit criteria
    if (currentDepth < 0)
    {
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
    // As of this point it is the nearest hit voxel

    // Set the data that comes from the ray intersection test
    setRayDepth(texelCoord, hit.dist); // Update the nearest distance
    setFirstHitOldDepth(texelCoord, hit.dist);
    setFirstHitPosition(texelCoord, hit.hitLocation);
    setFirstHitNormal(texelCoord, normalize(hit.normal));

    // Calculate motion vectors
    {
        // If it gets here, then the motion vectors should be calcuated
        vec3 hitLocation = hit.voxelHitLocation;

        // Transform the hit location to world space, using historical information
        hitLocation -= vec3(cellCount); // This moves the origin of the voxel world to its center
        hitLocation *= pastVoxelWorldScale; // Apply the scale of the voxel world
        hitLocation = qtransform(pastVoxelWorldRotation, hitLocation); // Rotate back into world space
        hitLocation += pastVoxelWorldPosition; // Apply the voxel world position

        // Hit location is now in world space using historical information
        // Project into camera space using historical camera data

        hitLocation -= pastCameraPosition; // Place relative to camera
        hitLocation = qtransform(vec4(pastCameraRotation.xyz, -pastCameraRotation.w), hitLocation); // Rotate into camera space
        hitLocation.yz /= vec2(1, float(resolution.y) / resolution.x) * pastCameraFovTan; // Warp for fov

        hitLocation.yz /= hitLocation.x; // Put into camera space

        hitLocation = vec3(-hitLocation.y, hitLocation.z, hitLocation.x); // Blit the data
        // x = x screen space (possibly flipped, probably not)
        // y = y screen space
        // z = depth

        hitLocation.xy *= 0.5;
        hitLocation.xy += 0.5;
        // vec2 motionVector = (((vec2(texelCoord.xy)) / resolution.xy) - hitLocation.xy); // UNfortunately this suffers from floating point inaccuracy. (So when close by, it drifts)
        //  vec2 motionVector = hitLocation.xy;
        // setFirstHitMotionVector(texelCoord, motionVector);
        changeAccumulatingMotionVectors(texelCoord, 1 * (vec2(texelCoord.xy) - hitLocation.xy * resolution.xy));
        // setFirstHitMotionVector(texelCoord, vec2(texelCoord.xy) - hitLocation.xy * resolution.xy);
    }

    // Set the information that comes from material
    MaterialDefinition voxelMaterial = materialDefinitions[hit.material]; // Get the material index of the hit, and map it to an actual material

    vec2 uv = mod(hit.uv / vec2(voxelMaterial.textureScaleX, voxelMaterial.textureScaleY), 1);
    setFirstHitMaterialUV(texelCoord, uv);

    setFirstHitMaterial(texelCoord, int(hit.material));

    // Modify material data with textures
    {
        // Get the uv coord
        if (voxelMaterial.albedoTextureID != 0)
        {
            sampler2D albedoTexture = sampler2D(voxelMaterial.albedoTextureID);
            voxelMaterial.albedo *= texture(albedoTexture, uv).xyz;
        }

        if (voxelMaterial.roughnessTextureID != 0)
        {
            sampler2D roughnessTexture = sampler2D(voxelMaterial.roughnessTextureID);
            voxelMaterial.roughness *= texture(roughnessTexture, uv).x;
        }

        if (voxelMaterial.emissionTextureID != 0)
        {
            sampler2D emissionTexture = sampler2D(voxelMaterial.emissionTextureID);
            voxelMaterial.emission *= texture(emissionTexture, uv).xyz;
        }
    }

    // Choose next ray direction
    {
        float seed = random + float(texelCoord.x + resolution.x * (texelCoord.y)) / resolution.x / resolution.y; // texelCoord.x + texelCoord.y * 1.61803398875 + texelCoord.z * 3.1415926589;

        vec3 direction = rayDir; // The direction the ray cast was in

        vec3 position = hit.hitLocation;

        // Load the hit normal
        vec3 normal = hit.normal; // The normal direction of the hit

        normal = normalize(normal);
        direction = normalize(direction);

        vec4 nextDirection = sampleGGX2(voxelMaterial.roughness, randomVec2(seed), direction, normal);

        // If a sun ray could hit the sun, then try
        if (sunAngularSize > 0 && dot(normal, sunDirection) > 0.0)
        {
            float p = 0.1;
            float maxTheta = sunAngularSize * (3.1415926589 / 180.0) / 2.0;
            vec3 targetDir = normalize(sunDirection);

            if (randomVec2(seed).x < p)
            {
                float cosTheta = 1 - randomVec2(seed).x * (1 - cos(maxTheta));
                float theta = acos(cosTheta);
                float phi = randomVec2(seed).y * 2 * 3.1415926589;

                vec3 local = vec3(sin(theta), sin(theta), cos(theta)) * vec3(cos(phi), sin(phi), 1);

                vec3 up = abs(targetDir.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
                vec3 tangent = normalize(cross(up, targetDir));
                vec3 bitangent = cross(targetDir, tangent);

                vec3 dir = normalize(local.x * tangent + local.y * bitangent + local.z * targetDir);

                nextDirection = vec4(dir, 1 / (p / (2 * 3.1415926589 * (1 - cos(maxTheta)))) * dot(dir.xyz, normal));
            }
            else
            {
                nextDirection.w = 1 / ((1 - p) / nextDirection.w);
            }
        }

        setSecondaryDirection(texelCoord, vec4(normalize(nextDirection.xyz), nextDirection.w));
        setRayPosition(texelCoord, position); // Set where the ray should start from next
        setRayDirection(texelCoord, normalize(nextDirection.xyz)); // Set the direction the ray should start from next
    }
}
