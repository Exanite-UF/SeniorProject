#version 460 core


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

uniform ivec3 resolution; //(xSize, ySize, 1)
uniform vec4 cameraRotation;
uniform vec3 cameraPosition;


layout(std430, binding = 0) buffer AccumulatedLight
{
    float accumulatedLight[];
};

layout(std430, binding = 1) buffer FirstHitNormal
{
    float firstHitNormal[];
};

layout(std430, binding = 2) buffer FirstHitPosition
{
    float firstHitPosition[];
};

layout(std430, binding = 3) buffer FirstHitMisc
{
    float firstHitMisc[];
};



layout(std430, binding = 4) buffer FirstHitMaterial
{
    readonly int firstHitMaterial[];
};

int getFirstHitMaterial(ivec3 coord){
    int index = 1 * (coord.x + resolution.x * (coord.y)); // Stride of 1, axis order is x y
    return firstHitMaterial[index];
}

layout(std430, binding = 5) buffer PrimaryDirection
{
    readonly float primaryDirection[];
};

vec3 getPrimaryDirection(ivec3 coord){
    int index = 3 * (coord.x + resolution.x * (coord.y)); // Stride of 1, axis order is x y
    return vec3(primaryDirection[index + 0], primaryDirection[index + 1], primaryDirection[index + 2]);
}

layout(std430, binding = 6) buffer SecondaryDirection
{
    readonly float secondaryDirection[];
};

vec4 getSecondaryDirection(ivec3 coord){
    int index = 4 * (coord.x + resolution.x * (coord.y)); // Stride of 1, axis order is x y
    return vec4(secondaryDirection[index + 0], secondaryDirection[index + 1], secondaryDirection[index + 2], secondaryDirection[index + 3]);
}

// Each entry is 32 bytes long (There are 12 bytes of padding)
layout(std430, binding = 7) buffer MaterialDefinitions
{
    readonly restrict MaterialDefinition materialDefinitions[];
};





vec3 getLight(ivec3 coord)
{
    int index = 3 * (coord.x + resolution.x * coord.y); // Stride of 3, axis order is x y

    return vec3(accumulatedLight[0 + index], accumulatedLight[1 + index], accumulatedLight[2 + index]);
}

vec3 getNormal(ivec3 coord)
{
    int index = 3 * (coord.x + resolution.x * (coord.y)); // Stride of 3, axis order is x y

    return vec3(firstHitNormal[0 + index], firstHitNormal[1 + index], firstHitNormal[2 + index]);
}

vec3 getPosition(ivec3 coord)
{
    int index = 3 * (coord.x + resolution.x * (coord.y)); // Stride of 3, axis order is x y

    return vec3(firstHitPosition[0 + index], firstHitPosition[1 + index], firstHitPosition[2 + index]);
}

vec4 getMisc(ivec3 coord)
{
    int index = 4 * (coord.x + resolution.x * (coord.y)); // Stride of 3, axis order is x y

    return vec4(firstHitMisc[0 + index], firstHitMisc[1 + index], firstHitMisc[2 + index], firstHitMisc[3 + index]);
}

vec3 qtransform(vec4 q, vec3 v)
{
    return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + q.w * v);
}

vec3 hueToRGB(float hue)
{
    hue = mod(hue, 1.0);
    float r = abs(hue * 6.0 - 3.0) - 1.0;
    float g = 2.0 - abs(hue * 6.0 - 2.0);
    float b = 2.0 - abs(hue * 6.0 - 4.0);
    return clamp(vec3(r, g, b), 0.0, 1.0);
}

vec4 safeVec4(vec4 v, vec4 fallback)
{
    return vec4(
        isnan(v.r) ? fallback.r : v.r,
        isnan(v.g) ? fallback.g : v.g,
        isnan(v.b) ? fallback.b : v.b,
        isnan(v.a) ? fallback.a : v.a);
}

in vec2 uv;
layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec3 posBuffer;
layout(location = 2) out vec3 normalBuffer;
layout(location = 3) out vec4 miscBuffer;


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



uniform float sunAngularSize; // The angle of the sun in diameter
float sunSize = cos(sunAngularSize * 3.14159265 / 180.0);
uniform vec3 sunDir;
uniform float sunBrightness;
uniform vec3 skyColor;
uniform vec3 groundColor;

vec3 skyBox(vec3 rayDirection){
    if(dot(normalize(sunDir), normalize(rayDirection)) > sunSize){
        return sunBrightness / (6.28318530718 * (1 - sunSize)) * vec3(1, 1, 1);
    }else if(dot(rayDirection, vec3(0, 0, 1)) > 0){
        return skyColor;
    }else{
        return groundColor;
    }
}

void main()
{
    ivec3 texelCoord = ivec3(gl_FragCoord.xy, 0);

    ivec3 size = resolution; // imageSize(hitPosition);
    vec3 color = vec3(0);

    vec3 normal = getNormal(texelCoord);//worldspace
    vec3 position = getPosition(texelCoord);//worldspace
    vec4 misc = getMisc(texelCoord);

    
    

    vec3 light = vec3(0);
    float samples = 0;
    MaterialDefinition voxelMaterial = materialDefinitions[getFirstHitMaterial(texelCoord)]; // Get the material index of the hit, and map it to an actual material
    
    vec3 direction = getPrimaryDirection(texelCoord);

    int radius = 1;
    if(misc.x < 0.01){
        radius = 0;
    }
    //radius = 0;

    const float kernel[3] = float[3](1.0 / 4.0, 4.0 / 8.0, 1.0 / 4.0);
    for(int i = -radius; i <= radius; i++){
        for(int j = -radius; j <= radius; j++){
            ivec3 coord = texelCoord + ivec3(i, j, 0);// * abs(ivec3(i, j, 0));

            //If this is offscreen
            if(coord.x < 0 || coord.x >= resolution.x || coord.y < 0 || coord.y >= resolution.y){
                continue;
            }

            //If this pixel didn't have a secondary ray
            if(getMisc(coord).x < 0){
                continue;
            }

            vec3 sampleNormal = getNormal(coord);//worldspace
            if(dot(sampleNormal, normal) < 0.9){
                continue;
            }

            //Materials of different roughnesses have different bounce distributions, so they cannot be combined
            vec4 sampleMisc = getMisc(coord);
            if(abs(sampleMisc.x - misc.x) > 0.1){
                continue;
            }

            vec3 sampledLight = getLight(coord);//This is the radiance coming from the secondary rays
            vec4 nextDirection = getSecondaryDirection(coord);

            vec3 brdfValue = brdf2(normal, direction, nextDirection.xyz, voxelMaterial) * nextDirection.w;
            //vec3 brdfValue = dot(nextDirection.xyz, normal) * brdf(normal, direction, nextDirection.xyz, voxelMaterial) * nextDirection.w;
            float multiplier = kernel[i + 1] * kernel[j + 1];
            light += sampledLight * brdfValue * multiplier;
            samples += multiplier;
        }
    }

    normal = qtransform(vec4(-cameraRotation.xyz, cameraRotation.w), normal);
    // normal is now in camera space
    //(1, 0, 0) away from the camera
    //(0, 1, 0) to the left
    //(0, 0, 1) up

    vec3 firstHitEmission = vec3(0);
    if(misc.x >= 0){
        firstHitEmission = voxelMaterial.emission;
    }else{
        light *= 0;
        samples = 1;
        firstHitEmission = skyBox(direction);
    }

    fragColor = vec4(light / samples + firstHitEmission, 1);
    posBuffer = position;
    miscBuffer = misc;
    normalBuffer = normal;
}
