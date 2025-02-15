#version 440

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

struct MaterialProperties{
    vec3 emission;
    vec3 albedo;
    vec3 metallicAlbedo;
    float roughness;
    float metallic;
}




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

layout(std430, binding = 3) buffer PriorAttenuation
{
    uint priorAttenuation[];
};

//At the moment every material is fully defined using a texture
//As such 512 textures are sent in for each material property

//This is the data used to find the bindless textures
uniform Emissions
{
  sampler2D emissions[512];
};

uniform Albedos
{
  sampler2D albedos[512];
};

uniform MetallicAlbedos
{
  sampler2D metallicAlbedos[512];
};

//Roughness and Metallic
uniform RMTextures
{
  sampler2D rmTextures[512];
};

uniform uint materialMap[4096];//This maps from the material index from the ray cast to the index of an actual material
uniform vec2 sizes[512];//The scaling of each material tells us how large a pixel is in terms of voxels (0.5 means that two pixels have then same length as 1 voxel)
uniform float random;//This is used to make non-deterministic randomness

float hash(float seed) {
    return fract(sin((seed + random) * 12345.6789) * 43758.5453123);
}

vec2 randomVec2(float seed) {
    return vec2(hash(seed), hash(seed + 1.0));
}

//This has the correct distribution for a Labertian material
//It returns a direction, and a multiplier that needs to be applied to the final light intensity (It corrects for the sampling distribution. It is the reciprocal of the PDF of the distribution)
vec4 randomHemisphereDirection(vec3 normal, vec2 rand) {
    float theta = acos(sqrt(rand.x)); // Cosine-weighted distribution (I don't know why this make the PDF equal to cos(theta), but the internet says is does)
    float phi = 2.0 * 3.14159265359 * rand.y; // Uniform azimuthal angle

    // Convert to Cartesian coordinates
    float x = cos(phi) * sin(theta);
    float y = sin(phi) * sin(theta);
    float z = cos(theta);

    // Convert from local (z-up) space to world space
    vec3 temp = abs(normal.z) < 0.5 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);//Pick a direction not parallel with the normal (Either pick up if it is definitely not facing up, or pick along the x axis if it is definitely pointing up)
    vec3 tangent = normalize(cross(temp, normal));//Get a vector perpecdicular to the previous two
    vec3 bitangent = cross(normal, tangent);//Get a second vector that is perpendicular to the previous 2
    //We now have three vectors: the normal direction, perpedicular to the normal, and other perpendicular to the normal
    
    vec3 dir = tangent * x + bitangent * y + normal * z;//This is a transformation between coordinate systems
    
    return vec4(dir, 3.1415926589 / dot(normal, dir));
}

//This is a uniform distribution
//It returns a direction, and a multiplier that needs to be applied to the final light intensity (It corrects for the sampling distribution. It is the reciprocal of the PDF of the distribution)
vec4 randomHemisphereDirectionUniform(vec3 normal, vec2 rand) {
    float theta = acos(1.0 - rand.x); // Uniform distribution
    float phi = 2.0 * 3.14159265359 * rand.y;

    float x = cos(phi) * sin(theta);
    float y = sin(phi) * sin(theta);
    float z = cos(theta);

    // Convert from local (z-up) space to world space
    vec3 temp = abs(normal.z) < 0.5 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);//Pick a direction not parallel with the normal (Either pick up if it is definitely not facing up, or pick along the x axis if it is definitely pointing up)
    vec3 tangent = normalize(cross(temp, normal));//Get a vector perpecdicular to the previous two
    vec3 bitangent = cross(normal, tangent);//Get a second vector that is perpendicular to the previous 2
    //We now have three vectors: the normal direction, perpedicular to the normal, and other perpendicular to the normal

    return vec4(tangent * x + bitangent * y + normal * z, 6.28318530718);
}

float microfacetDistribution(float dotOfNormalAndHalfway, float roughness){
    float rough2 = roughness * roughness;
    float temp = dotOfNormalAndHalfway * dotOfNormalAndHalfway * (rough2 - 1) + 1;
    return rough2 / (3.1415926589 * temp * temp);
}

//For metals baseReflectivity is the metallicAlbedo
//For non-metals, it is a completely different property (I'm going to go with 1)
//So this parameter will be a linear interpolation based on the metallic parameter between 1 and the metallicAlbedo
vec3 fresnel(float dotOfViewAndHalfway, vec3 baseReflectivity){
    return baseReflectivity + (1 - baseReflectivity) * pow(1 - dotOfViewAndHalfway, 5);//Schlick’s approximation
}

float geometricBlocking(float dotOfViewAndNormal, float dotOfLightAndNormal, float roughness){
    float something = (roughness + 1) * (roughness + 1) / 8;//I don't know what this represents (its usually called k)
    float factor1 = dotOfViewAndNormal / (dotOfViewAndNormal * (1 - something) + something);
    float factor2 = dotOfLightAndNormal / (dotOfLightAndNormal * (1 - something) + something);

    return factor1 * factor2;
}

//view is the direction we came from
//light is the direction we are going
//These names come from standard terminology for the subject
vec3 brdf(vec3 normal, vec3 view, vec3 light, MaterialProperties voxelMaterial){
    vec3 halfway = normalize(incoming + outgoing);

    vec3 baseReflectivity = vec3(1 - voxelMaterial.metallic) + voxelMaterial.metallic * voxelMaterial.metallicAlbedo;
    float microfacetComponent = microfacetDistribution(dot(halfway, normal), roughness);
    vec3 fresnelComponent = fresnel(dot(view, halfway), baseReflectivity);
    float geometricComponent = geometricBlocking(dot(view, normal), dot(light, normal), voxelMaterial.roughness);

    //The effect of the metallicAlbedo is performed in the fresenl component
    //A non metallic material is assumed to not be affected by the fresnel effect
    //The fresnel component will color the reflected light, and will behave according to an approximation of the fresnel effect
    //Since albedo is about perfectly diffuse refection color, that would imply no fresnel effect
    //This does mean that there are two ways to get a colored mirror reflection. (one with and one without the fresnel effect)
    //It also means that the color of the metallic albedo will show off stronger at sharper angles
    //We add the metallic value to the albedo to prevent darkening. (this is a multiplier, so not doing this would just make metals black)
    vec3 albedo = (1 - voxelMaterial.metallic) * voxelMaterial.albedo + voxelMaterial.metallic;

    return microfacetComponent * fresnelComponent * geometricComponent * albedo;
}



void main()
{
    ivec3 texelCoord = ivec3(gl_GlobalInvocationID.xyz);
    float seed = texelCoord.x + texelCoord.y * 1.61803398875 + texelCoord.z * 3.1415926589;

    //int material = materialMap[hitMaterial[/*material index*/]]//Get the material index of the hit, and map it to an actual material

    /*
    The position on the plane of the intersection in voxel space, is needed
      So if the xy plane is perpendicular to the hitNormal, then we need the x and y coordinates of the hit location in voxel space
      This would be most efficiently done in the ExecuteRayTrace shader, by storing that result
      This value is needed to calulate the uv of the material textures
      I will call this hitUV
    */
    
    vec3 attentuation;//This is the accumulated attenuation

    float dist;//Distance that the ray cast covered
    vec3 normal;//The normal direction of the hit
    vec3 direction;//The direction the ray cast was in


    //vec2 hitUV = /*Not implemented*/
    vec2 uv = hitUV * sizes[material]

    MaterialProperties voxelMaterial;
    voxelMaterial.emission = texture(sampler2d(emissions[material]), uv).xyz;
    voxelMaterial.albedo = texture(sampler2d(albedos[material]), uv).xyz;
    voxelMaterial.metallicAlbedo = texture(sampler2d(albedos[material]), uv).xyz;
    vec4 rmTexture = texture(sampler2d(rmTextures[material]), uv);
    voxelMaterial.roughness = rmTexture.r;
    voxelMaterial.metallic = rmTexture.g;
    //Lambertian gets divided by pi

    vec3 nextDirection = randomHemisphereDirection(randomVec2(seed));//Get the next direction to sample in

    vec3 brdfValue = brdf(normal, direction, nextDirection, voxelMaterial);
    //Light falloff is a consequence of the integral in the rendering equation.
    //Point sources of light don't exist.
    //They would have infinite radiance since their solid angle is 0. Any amount of light coming from 0 steradians and 0 projected area, will be infinite.
    //If you do give a point source of light a finite radiance it would be pitch black. If it had infinite radiance, then the amount of light you should see is indeterminate.

    //So long as our lights have actuall size, light falloff is natural consequence and is not something to be explicitly caused.
    //Though if you do use a point source, then you would use the inverse square law on what is actually a radiant intesity value.
    //  You calculate the surface area of the spherical arc at the distance away.
    //  So a spot light confines its beam to a spherical arc of 1 steradian. You divide the radiant intensity by the surface area of the 1 steradian spherical arc when at the given distance away.
    //  This has the effect of infinite brightness as you get closer. To prevent this you can add 1 to the surface area before dividing.
    //  4 * pi * r^2 is the surface area of a sphere
    //  steradians * r^2 is the surface are of a spherical arc of a given steradians

    vec3 receivedLight = voxelMaterial.emission * attentuation;
    vec3 nextAttenuation = brdfValue;

}
