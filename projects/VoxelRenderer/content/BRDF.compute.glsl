#version 440

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

//Padded to 32 bytes long
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

layout(std430, binding = 0) buffer HitPosition
{
    vec4 hitPosition[];
};
layout(std430, binding = 1) buffer HitNormal
{
    vec4 hitNormal[];
};

layout(std430, binding = 2) buffer HitMaterial
{
    uint hitMaterial[];
};

layout(std430, binding = 3) buffer HitVoxelPosition
{
    float hitVoxelPosition[];
};

layout(std430, binding = 4) buffer RayPosition
{
    float rayPosition[];
};

layout(std430, binding = 5) buffer RayDirection
{
    float rayDirection[];
};

layout(std430, binding = 6) buffer PriorAttenuation
{
    float priorAttenuation[];
};

layout(std430, binding = 7) buffer AccumulatedLight
{
    float accumulatedLight[];
};

// At the moment every material is fully defined using a texture
// As such 512 textures are sent in for each material property

// This struct is 40 bytes long (The data is tightly packed)
//struct MaterialTextureSet
//{
//    sampler2D emission; // uint64_t
//    sampler2D albedo;
//    sampler2D metallicAlbedo;
//    sampler2D rmTexture; // Roughness and Metallic
//    vec2 size; // The scaling of each material tells us what percent of a texture each voxel is when measured linearly.//This is 2 floats (and it is packed dense)
//};

layout(std430, binding = 8) buffer MaterialMap{
    uint materialMap[]; // This maps from the material index from the ray cast to the index of an actual material
};
// Each entry is 32 bytes long (There are 12 bytes of padding)
layout(std430, binding = 9) buffer MaterialBases{
    MaterialProperties materialBases[];//This is the base colors of the materials
};
// Each entry is 48 bytes long (There are 8 bytes of padding)
//layout(std430, binding = 10) buffer MaterialTextures{
//    MaterialTextureSet materialTextures[];// This is the data used to find the bindless textures
//};

uniform uint materialMapSize;
uniform uint materialCount;



uniform ivec3 resolution; //(xSize, ySize, raysPerPixel)


uniform float random; // This is used to make non-deterministic randomness

// Buffer access and set

vec4 getHitPosition(ivec3 coord)
{
    int index = (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // axis order is x y z

    return hitPosition[index];
}

vec4 getHitNormal(ivec3 coord)
{
    int index = (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // axis order is x y z

    return hitNormal[index];
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

vec3 getPriorAttenuation(ivec3 coord)
{
    int index = 3 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride is 3, axis order is x y z

    return vec3(priorAttenuation[index + 0], priorAttenuation[index + 1], priorAttenuation[index + 2]);
}

void setHitPosition(ivec3 coord, vec4 value)
{
    int index = (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride of 3, axis order is x y z
    hitPosition[index] = value;
}

void setHitNormal(ivec3 coord, vec4 value)
{
    int index = (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride of 3, axis order is x y z
    hitNormal[index] = value;
}

void setHitMaterial(ivec3 coord, uint value)
{
    int index = (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride of 3, axis order is x y z
    hitMaterial[index] = value;
}

void setHitVoxelPosition(ivec3 coord, vec3 value)
{
    int index = 3 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride of 3, axis order is x y z
    hitVoxelPosition[index + 0] = value.x;
    hitVoxelPosition[index + 1] = value.y;
    hitVoxelPosition[index + 2] = value.z;
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

void clearZBuffer(ivec3 coord)
{
     int index = (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // axis order is x y z

    hitNormal[index].w = 1.0 / 0.0;
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

// This has the correct distribution for a Labertian material
// It returns a direction, and a multiplier that needs to be applied to the final light intensity (It corrects for the sampling distribution. It is the reciprocal of the PDF of the distribution)
vec4 randomHemisphereDirection(vec3 normal, vec2 rand)
{
    float theta = acos(sqrt(rand.x)); // Cosine-weighted distribution (I don't know why this make the PDF equal to cos(theta), but the internet says is does)
    float phi = 2.0 * 3.14159265359 * rand.y; // Uniform azimuthal angle

    // Convert to Cartesian coordinates
    float x = cos(phi) * sin(theta);
    float y = sin(phi) * sin(theta);
    float z = cos(theta);

    // Convert from local (z-up) space to world space
    vec3 temp = abs(normal.z) < 0.5 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0); // Pick a direction not parallel with the normal (Either pick up if it is definitely not facing up, or pick along the x axis if it is definitely pointing up)
    vec3 tangent = normalize(cross(temp, normal)); // Get a vector perpecdicular to the previous two
    vec3 bitangent = cross(normal, tangent); // Get a second vector that is perpendicular to the previous 2
    // We now have three vectors: the normal direction, perpedicular to the normal, and other perpendicular to the normal

    vec3 dir = tangent * x + bitangent * y + normal * z; // This is a transformation between coordinate systems

    return vec4(dir, 3.1415926589 / dot(normal, dir));
}

// This is a uniform distribution
// It returns a direction, and a multiplier that needs to be applied to the final light intensity (It corrects for the sampling distribution. It is the reciprocal of the PDF of the distribution)
vec4 randomHemisphereDirectionUniform(vec3 normal, vec2 rand)
{
    float theta = acos(1.0 - rand.x); // Uniform distribution
    float phi = 2.0 * 3.14159265359 * rand.y;

    float x = cos(phi) * sin(theta);
    float y = sin(phi) * sin(theta);
    float z = cos(theta);

    // Convert from local (z-up) space to world space
    vec3 temp = abs(normal.z) < 0.5 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0); // Pick a direction not parallel with the normal (Either pick up if it is definitely not facing up, or pick along the x axis if it is definitely pointing up)
    vec3 tangent = normalize(cross(temp, normal)); // Get a vector perpecdicular to the previous two
    vec3 bitangent = cross(normal, tangent); // Get a second vector that is perpendicular to the previous 2
    // We now have three vectors: the normal direction, perpedicular to the normal, and other perpendicular to the normal

    return vec4(tangent * x + bitangent * y + normal * z, 6.28318530718);
}

// This is the GGX distribution
float microfacetDistribution(float dotOfNormalAndHalfway, float roughness)
{
    float rough2 = roughness * roughness * roughness * roughness;
    float temp = dotOfNormalAndHalfway * dotOfNormalAndHalfway * (rough2 - 1) + 1;
    return rough2 / (3.1415926589 * temp * temp);
}

// This is the GGX
// It returns a direction, and a multiplier that needs to be applied to the final light intensity (It corrects for the sampling distribution. It is the reciprocal of the PDF of the distribution)
vec4 randomHemisphereDirectionGGX(vec3 normal, vec2 rand, float roughness)
{
    // Random azimuthal angle (in [0, 2*pi])
    float phi = 2.0 * 3.14159 * rand.x;

    // Compute polar angle (in [0, pi])
    float cosTheta = sqrt((1.0 - rand.y) / (1.0 + (roughness * roughness - 1.0) * rand.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    // Convert from local (z-up) space to world space
    vec3 temp = abs(normal.z) < 0.5 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0); // Pick a direction not parallel with the normal (Either pick up if it is definitely not facing up, or pick along the x axis if it is definitely pointing up)
    vec3 tangent = normalize(cross(temp, normal)); // Get a vector perpecdicular to the previous two
    vec3 bitangent = cross(normal, tangent); // Get a second vector that is perpendicular to the previous 2
    // We now have three vectors: the normal direction, perpedicular to the normal, and other perpendicular to the normal

    // Now we can compute the final direction
    vec3 direction = cos(phi) * sinTheta * tangent + sin(phi) * sinTheta * bitangent + cosTheta * normal;

    return vec4(direction, 1.0 / microfacetDistribution(dot(normalize(normal + direction), normal), roughness));
}

// For metals baseReflectivity is the metallicAlbedo
// For non-metals, it is a completely different property (I'm going to go with 1)
// So this parameter will be a linear interpolation based on the metallic parameter between 1 and the metallicAlbedo
vec3 fresnel(float dotOfViewAndHalfway, vec3 baseReflectivity)
{
    return baseReflectivity + (1 - baseReflectivity) * pow(1 - dotOfViewAndHalfway, 5); // Schlick’s approximation
}

// This is also based on GGX
float geometricBlocking(float dotOfViewAndNormal, float dotOfLightAndNormal, float roughness)
{
    float something = (roughness + 1) * (roughness + 1) / 8; // I don't know what this represents (its usually called k)
    // something can also be roughness / 2
    // But that is a worse approximation of the underlying function
    float factor1 = max(dotOfViewAndNormal, 0) / (dotOfViewAndNormal * (1 - something) + something);
    float factor2 = max(dotOfLightAndNormal, 0) / (dotOfLightAndNormal * (1 - something) + something);

    return factor1 * factor2;
}

// view is the direction we came from
// light is the direction we are going
// These names come from standard terminology for the subject
vec3 brdf(vec3 normal, vec3 view, vec3 light, MaterialProperties voxelMaterial)
{
    vec3 halfway = normalize(view + light); // This is used by several things

    vec3 baseReflectivity = vec3(1 - voxelMaterial.metallic) + voxelMaterial.metallic * voxelMaterial.metallicAlbedo; // This is the metallic reflectivity (For non-metallic materials it is 1, for metallic materials is it the metallicAlbedo)

    // Since the microfacet distribution is equal to the sampling distribution, this factors cancels out with the division by the sampling distribution
    // GGX has a specific sampling distribution to use, and it is equal to the microfacet distribution
    float microfacetComponent = 1;//microfacetDistribution(dot(halfway, normal), voxelMaterial.roughness);//This is the component of the BRDF that accounts for the direction of microfacets (Based on the distribution of microfacet directions, what is the percent of light that reflects toward the camera)

    vec3 fresnelComponent = vec3(1);//fresnel(dot(view, halfway), baseReflectivity); // This component simulates the fresnel effect (only metallic materials have this)

    // This approximates how much light is blocked by microfacets, when looking from different directions
    float dotOfViewAndNormal = dot(view, normal);
    float dotOfLightAndNormal = dot(light, normal);
    float geometricComponent = geometricBlocking(abs(dotOfViewAndNormal), abs(dotOfLightAndNormal), voxelMaterial.roughness); // Like how a mountain blocks the light in a valley

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

void main()
{
    ivec3 texelCoord = ivec3(gl_GlobalInvocationID.xyz);
    float seed = random + float(texelCoord.x + resolution.x * (texelCoord.y + resolution.y * texelCoord.z)) / resolution.x / resolution.y / resolution.z;//texelCoord.x + texelCoord.y * 1.61803398875 + texelCoord.z * 3.1415926589;

    uint materialID = materialMap[getHitMaterial(texelCoord)]; // Get the material index of the hit, and map it to an actual material

    // Load the hit position
    vec4 temp = getHitPosition(texelCoord);
    bool wasHit = temp.w != 0;
    vec3 position = temp.xyz;

    // Load the hit normal
    temp = getHitNormal(texelCoord);
    float dist = temp.w; // Distance that the ray cast covered
    vec3 normal = temp.xyz; // The normal direction of the hit

    vec3 attentuation = getPriorAttenuation(texelCoord); // This is the accumulated attenuation
    vec3 direction = normalize(getDirection(texelCoord)); // The direction the ray cast was in

    // Find the uv coordinate for the texture
    // It is based on the hit location in voxel space
    vec3 voxelPosition = getHitVoxelPosition(texelCoord);
    vec2 hitUV;
    if (abs(normal.x) > 0)
    {
        // yz
        hitUV = voxelPosition.yz;
    }
    else if (abs(normal.y) > 0)
    {
        // xz
        hitUV = voxelPosition.xz;
    }
    else if (abs(normal.z) > 0)
    {
        // xy
        hitUV = voxelPosition.xy;
    }
    //vec2 uv = hitUV * sizes[material]; // We need to set the material textures to SL_REPEAT mode (this is the default).

    // Format the voxel material into a struct
    // Load the correct material values from the array of material textures
    voxelMaterial = materialBases[materialID];//This is the base value of the material

    //Multiply in the texture values
    /*
    voxelMaterial.emission *= texture(sampler2d(materialTextures[materialID].emission), uv).xyz;
    voxelMaterial.albedo *= texture(sampler2d(materialTextures[materialID].albedo), uv).xyz;
    voxelMaterial.metallicAlbedo *= texture(sampler2d(materialTextures[materialID].metallicAlbedo), uv).xyz;
    vec4 rmTexture = texture(sampler2d(materialTextures[materialID].rmTexture), uv);
    voxelMaterial.roughness *= rmTexture.r;
    voxelMaterial.metallic *= rmTexture.g;
    */


    //vec4 nextDirection = randomHemisphereDirectionUniform(normal, randomVec2(seed));//randomHemisphereDirectionGGX(normal, randomVec2(seed), voxelMaterial.roughness); // Get the next direction to sample in
    vec4 nextDirection = randomHemisphereDirectionGGX(normal, randomVec2(seed), voxelMaterial.roughness); // Get the next direction to sample in

    // Calculate the BRDF

    // Usually we would divide by the sampling distribution (I already calculated the reciprocal), but in this case the sampling distribution is equal to the microfacet distribution that is used inside this function, so they end up cancelling out
    
    //This is how the brdf is usually calculated
    //vec3 brdfValue = dot(nextDirection.xyz, normal) * brdf(normal, direction, nextDirection.xyz, voxelMaterial) * nextDirection.w;// store the factor that needs to be multiplied by the BRDF to cancel out the bias caused by a non-uniform sampling distribution
    
    //This works for GGX because of how the sampling distribution and BRDF are made
    //Note that the microfacet distribution is being ignored in the BRDF calculation to prevent ' * nextDirection.w' from needing to be run
    //The microfacet distribution and the sampling correction are reciprocals
    vec3 brdfValue = brdf(normal, direction, nextDirection.xyz, voxelMaterial);
    
    // Light falloff is a consequence of the integral in the rendering equation.
    // Point sources of light don't exist.
    // They would have infinite radiance since their solid angle is 0. Any amount of light coming from 0 steradians and 0 projected area, will be infinite.
    // If you do give a point source of light a finite radiance it would be pitch black. If it had infinite radiance, then the amount of light you should see is indeterminate.

    // So long as our lights have actuall size, light falloff is natural consequence and is not something to be explicitly caused.
    // Though if you do use a point source, then you would use the inverse square law on what is actually a radiant intesity value.
    //   You calculate the surface area of the spherical arc at the distance away.
    //   So a spot light confines its beam to a spherical arc of 1 steradian. You divide the radiant intensity by the surface area of the 1 steradian spherical arc when at the given distance away.
    //   This has the effect of infinite brightness as you get closer. To prevent this you can add 1 to the surface area before dividing.
    //   4 * pi * r^2 is the surface area of a sphere
    //   steradians * r^2 is the surface are of a spherical arc of a given steradians

    vec3 receivedLight = voxelMaterial.emission * attentuation;

    // Set the output buffers
    setPosition(texelCoord, position); // Set where the ray should start from next
    setDirection(texelCoord, normalize(nextDirection.xyz)); // Set the direction the ray should start from next

    setHitPosition(texelCoord, vec4(0));
    setHitNormal(texelCoord, vec4(0, 0, 0, 1.0/0.0));
    setHitVoxelPosition(texelCoord, vec3(0));
    setHitMaterial(texelCoord, 0);

    if(wasHit){
        setAttenuation(texelCoord, attentuation * brdfValue); // The attenuation for the next bounce is the current attenuation times the brdf
        changeLightAccumulation(texelCoord, receivedLight); // Accumulate the light the has reached the camera
    }else{
        //Nothing was hit
        //changeLightAccumulation(texelCoord, dot(direction, normalize(vec3(1, 1, 1))) * vec3(1, 1, 0) * attentuation);
        if(dot(direction, normalize(vec3(1, 1, 1))) > 0.9){
            changeLightAccumulation(texelCoord, vec3(1, 1, 0) * attentuation); //And there is no light from this direction
        }else{
            changeLightAccumulation(texelCoord, vec3(0)); //And there is no light from this direction
        }
        setAttenuation(texelCoord, vec3(0));//No more light can come
    }
    
}
