#version 460 core

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

layout(std430, binding = 4) buffer FirstHitEmission
{
    float firstHitEmission[];
};

layout(std430, binding = 5) buffer FirstHitAttenuation
{
    float firstHitAttenuation[];
};

uniform ivec3 resolution; //(xSize, ySize, 1)
uniform vec4 cameraRotation;
uniform vec3 cameraPosition;

vec3 getFirstHitAttenuation(ivec3 coord)
{
    int index = 3 * (coord.x + resolution.x * coord.y); // Stride of 3, axis order is x y z
    return vec3(firstHitAttenuation[0 + index], firstHitAttenuation[1 + index], firstHitAttenuation[2 + index]);
}

vec3 getFirstHitEmission(ivec3 coord)
{
    int index = 3 * (coord.x + resolution.x * coord.y); // Stride of 3, axis order is x y z
    return vec3(firstHitEmission[0 + index], firstHitEmission[1 + index], firstHitEmission[2 + index]);
}

vec3 getLight(ivec3 coord)
{
    int index = 3 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride of 3, axis order is x y

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

void main()
{
    ivec3 size = resolution; // imageSize(hitPosition);
    vec3 color = vec3(0);

    vec3 normal = getNormal(ivec3(gl_FragCoord.xy, 0));
    vec3 position = getPosition(ivec3(gl_FragCoord.xy, 0));
    vec4 material = getMisc(ivec3(gl_FragCoord.xy, 0));

    normal = qtransform(vec4(-cameraRotation.xyz, cameraRotation.w), normal);
    // normal is now in camera space
    //(1, 0, 0) away from the camera
    //(0, 1, 0) to the left
    //(0, 0, 1) up

    // Put this into a frame buffer (an actual framebuffer)
    // And apply an anisotropic blur using the normal

    // position = qtransform(vec4(-cameraRotation.xyz, cameraRotation.w), position - cameraPosition);
    //  At this point position is in camera space
    //+x is in front of the camera
    //+y is to the left of the camera
    //+z is above the camera

    // position = position - cameraPosition;

    ivec3 texelCoord = ivec3(gl_FragCoord.xy, 0);

    vec3 light = getLight(texelCoord);//This is the radiance coming from the secondary rays


    fragColor = vec4(light * getFirstHitAttenuation(texelCoord) + getFirstHitEmission(texelCoord), 1);
    posBuffer = position;
    miscBuffer = material;
    normalBuffer = normal;
}
