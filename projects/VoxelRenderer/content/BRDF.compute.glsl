#version 440

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

// Padded to 32 bytes long
struct MaterialProperties
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
} voxelMaterial;

layout(std430, binding = 0) readonly buffer HitPosition
{
    float hitPosition[];
};
layout(std430, binding = 1) readonly buffer HitNormal
{
    float hitNormal[];
};

layout(std430, binding = 2) readonly buffer HitMaterial
{
    uint hitMaterial[];
};

layout(std430, binding = 3) readonly buffer HitVoxelPosition
{
    float hitVoxelPosition[];
};

layout(std430, binding = 10) restrict buffer HitMisc
{
    float hitMisc[];
};

layout(std430, binding = 4) restrict buffer RayPosition
{
    float rayPosition[];
};

layout(std430, binding = 5) restrict buffer RayDirection
{
    float rayDirection[];
};

layout(std430, binding = 6) restrict buffer PriorAttenuation
{
    float priorAttenuation[];
};

layout(std430, binding = 7) restrict buffer AccumulatedLight
{
    float accumulatedLight[];
};

// At the moment every material is fully defined using a texture
// As such 512 textures are sent in for each material property

// This struct is 40 bytes long (The data is tightly packed)
// struct MaterialTextureSet
//{
//    sampler2D emission; // uint64_t
//    sampler2D albedo;
//    sampler2D metallicAlbedo;
//    sampler2D rmTexture; // Roughness and Metallic
//    vec2 size; // The scaling of each material tells us what percent of a texture each voxel is when measured linearly.//This is 2 floats (and it is packed dense)
//};

layout(std430, binding = 8) restrict buffer MaterialMap
{
    uint materialMap[]; // This maps from the material index from the ray cast to the index of an actual material
};
// Each entry is 32 bytes long (There are 12 bytes of padding)
layout(std430, binding = 9) restrict buffer MaterialBases
{
    MaterialProperties materialBases[]; // This is the base colors of the materials
};
// Each entry is 48 bytes long (There are 8 bytes of padding)
// layout(std430, binding = 10) buffer MaterialTextures{
//    MaterialTextureSet materialTextures[];// This is the data used to find the bindless textures
//};

uniform uint materialMapSize;
uniform uint materialCount;

uniform ivec3 resolution; //(xSize, ySize, raysPerPixel)

uniform float random; // This is used to make non-deterministic randomness

// Buffer access and set

vec3 getHitPosition(ivec3 coord)
{
    int index = 3 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // axis order is x y z
    return vec3(hitPosition[0 + index], hitPosition[1 + index], hitPosition[2 + index]);
}

vec3 getHitNormal(ivec3 coord)
{
    int index = 3 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // axis order is x y z
    return vec3(hitNormal[0 + index], hitNormal[1 + index], hitNormal[2 + index]);
}

uint getHitMaterial(ivec3 coord)
{
    int index = (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // axis order is x y z

    return hitMaterial[index];
}

vec3 getHitVoxelPosition(ivec3 coord)
{
    int index = 3 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride of 3, axis order is x y

    return vec3(hitVoxelPosition[0 + index], hitVoxelPosition[1 + index], hitVoxelPosition[2 + index]);
}

bool getHitWasHit(ivec3 coord)
{
    int index = 2 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // axis order is x y z
    return hitMisc[index + 0] > 0;
}

float getHitDist(ivec3 coord)
{
    int index = 2 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // axis order is x y z
    return hitMisc[index + 1];
}

vec3 getPriorAttenuation(ivec3 coord)
{
    int index = 3 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride is 3, axis order is x y z

    return vec3(priorAttenuation[index + 0], priorAttenuation[index + 1], priorAttenuation[index + 2]);
}

void setHitWasHit(ivec3 coord, bool value)
{
    int index = 2 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // axis order is x y z
    hitMisc[index + 0] = (value) ? 1.0 : 0.0;
}

void setHitDist(ivec3 coord, float value)
{
    int index = 2 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // axis order is x y z
    hitMisc[index + 1] = value;
}

vec3 getDirection(ivec3 coord)
{
    int index = 3 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride of 3, axis order is x y

    return vec3(rayDirection[0 + index], rayDirection[1 + index], rayDirection[2 + index]);
}

void setAttenuation(ivec3 coord, vec3 value)
{
    int index = 3 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride is 3, axis order is x y z

    priorAttenuation[0 + index] = value.x;
    priorAttenuation[1 + index] = value.y;
    priorAttenuation[2 + index] = value.z;
}

void setPosition(ivec3 coord, vec3 value)
{
    int index = 3 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride of 3, axis order is x y z
    rayPosition[0 + index] = value.x;
    rayPosition[1 + index] = value.y;
    rayPosition[2 + index] = value.z;
}

void setDirection(ivec3 coord, vec3 value)
{
    int index = 3 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride of 3, axis order is x y z
    rayDirection[0 + index] = value.x;
    rayDirection[1 + index] = value.y;
    rayDirection[2 + index] = value.z;
}

void changeLightAccumulation(ivec3 coord, vec3 deltaValue)
{
    int index = 3 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride of 3, axis order is x y z
    accumulatedLight[0 + index] += deltaValue.x;
    accumulatedLight[1 + index] += deltaValue.y;
    accumulatedLight[2 + index] += deltaValue.z;
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
vec3 fresnel(float dotOfViewAndHalfway, vec3 baseReflectivity)
{
    return baseReflectivity + (1 - baseReflectivity) * pow(1 - dotOfViewAndHalfway, 5); // Schlick’s approximation
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

vec4 sampleGGX(float roughness, vec2 rand, vec3 view, vec3 normal)
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
        return vec4(light, 1 / (microfacetDistributionGGX(voxelMaterial.roughness, dot(normal, halfway)) * dot(microfacetDirection, normal) / abs(4 * dot(view, microfacetDirection))));
    }
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

vec3 brdf(vec3 normal, vec3 view, vec3 light, MaterialProperties voxelMaterial)
{
    vec3 halfway = normalize(view + light); // This is used by several things

    float microfacetComponent = microfacetDistributionGGX(voxelMaterial.roughness, dot(normal, halfway)); // This is the component of the BRDF that accounts for the direction of microfacets (Based on the distribution of microfacet directions, what is the percent of light that reflects toward the camera)

    vec3 baseReflectivity = vec3(1 - voxelMaterial.metallic) + voxelMaterial.metallic * voxelMaterial.metallicAlbedo; // This is the metallic reflectivity (For non-metallic materials it is 1, for metallic materials is it the metallicAlbedo)

    vec3 fresnelComponent = fresnel(dot(view, halfway), baseReflectivity); // This component simulates the fresnel effect (only metallic materials have this)

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

vec3 brdf2(vec3 normal, vec3 view, vec3 light, MaterialProperties voxelMaterial)
{
    vec3 halfway = normalize(view + light); // This is used by several things

    // This component is part of the sampling distribution pdf, so it cancels out
    // float microfacetComponent = microfacetDistributionGGX(voxelMaterial.roughness, dot(normal, halfway));//This is the component of the BRDF that accounts for the direction of microfacets (Based on the distribution of microfacet directions, what is the percent of light that reflects toward the camera)

    vec3 baseReflectivity = vec3(1 - voxelMaterial.metallic) + voxelMaterial.metallic * voxelMaterial.metallicAlbedo; // This is the metallic reflectivity (For non-metallic materials it is 1, for metallic materials is it the metallicAlbedo)

    vec3 fresnelComponent = fresnel(dot(view, halfway), baseReflectivity); // This component simulates the fresnel effect (only metallic materials have this)

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

void main()
{
    ivec3 texelCoord = ivec3(gl_GlobalInvocationID.xyz);
    float seed = random + float(texelCoord.x + resolution.x * (texelCoord.y + resolution.y * texelCoord.z)) / resolution.x / resolution.y / resolution.z; // texelCoord.x + texelCoord.y * 1.61803398875 + texelCoord.z * 3.1415926589;

    vec3 attentuation = getPriorAttenuation(texelCoord); // This is the accumulated attenuation
    vec3 direction = normalize(getDirection(texelCoord)); // The direction the ray cast was in

    // Load the hit position
    bool wasHit = getHitWasHit(texelCoord);

    setHitWasHit(texelCoord, false);
    if (!wasHit)
    {
        // Nothing was hit
        // changeLightAccumulation(texelCoord, dot(direction, normalize(vec3(1, 1, 1))) * vec3(1, 1, 0) * attentuation);
        if (dot(direction, normalize(vec3(1, 1, 1))) > 0.9)
        {
            changeLightAccumulation(texelCoord, vec3(10, 10, 0) * attentuation); // The sun
        }
        else
        {
            changeLightAccumulation(texelCoord, vec3(0)); // And there is no light from this direction
        }
        setAttenuation(texelCoord, vec3(0)); // No more light can come

        setHitDist(texelCoord, -1.0); // Set the hit distance such that early stopping will occur in the ray cast.
        return;
    }

    vec3 position = getHitPosition(texelCoord);

    // Load the hit normal
    // float dist = getHitDist(texelCoord); // Distance that the ray cast covered
    vec3 normal = getHitNormal(texelCoord); // The normal direction of the hit

    uint materialID = materialMap[getHitMaterial(texelCoord)]; // Get the material index of the hit, and map it to an actual material
    // Find the uv coordinate for the texture
    // It is based on the hit location in voxel space
    // vec3 voxelPosition = getHitVoxelPosition(texelCoord);
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
    voxelMaterial = materialBases[materialID]; // This is the base value of the material

    // Multiply in the texture values
    /*
    voxelMaterial.emission *= texture(sampler2d(materialTextures[materialID].emission), uv).xyz;
    voxelMaterial.albedo *= texture(sampler2d(materialTextures[materialID].albedo), uv).xyz;
    voxelMaterial.metallicAlbedo *= texture(sampler2d(materialTextures[materialID].metallicAlbedo), uv).xyz;
    vec4 rmTexture = texture(sampler2d(materialTextures[materialID].rmTexture), uv);
    voxelMaterial.roughness *= rmTexture.r;
    voxelMaterial.metallic *= rmTexture.g;
    */

    vec4 nextDirection = sampleGGX2(voxelMaterial.roughness, randomVec2(seed), direction, normal);
    vec3 brdfValue = brdf2(normal, direction, nextDirection.xyz, voxelMaterial) * nextDirection.w;

    // vec4 nextDirection = sampleGGX(voxelMaterial.roughness, randomVec2(seed), direction, normal);
    // vec3 brdfValue = dot(nextDirection.xyz, normal) * brdf(normal, direction, nextDirection.xyz, voxelMaterial) * nextDirection.w;

    vec3 receivedLight = voxelMaterial.emission * attentuation;

    // Set the output buffers
    setPosition(texelCoord, position); // Set where the ray should start from next
    setDirection(texelCoord, normalize(nextDirection.xyz)); // Set the direction the ray should start from next

    setHitDist(texelCoord, 1.0 / 0.0); // Set the hit distance to infinite
    setAttenuation(texelCoord, attentuation * brdfValue); // The attenuation for the next bounce is the current attenuation times the brdf
    changeLightAccumulation(texelCoord, receivedLight); // Accumulate the light the has reached the camera
}
