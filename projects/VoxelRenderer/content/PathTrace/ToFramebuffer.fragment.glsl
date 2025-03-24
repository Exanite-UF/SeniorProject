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

uniform ivec3 resolution; //(xSize, ySize, raysPerPixel)
uniform vec4 cameraRotation;
uniform vec3 cameraPosition;

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

vec3 getMisc(ivec3 coord)
{
    int index = 3 * (coord.x + resolution.x * (coord.y)); // Stride of 3, axis order is x y

    return vec3(firstHitMisc[0 + index], firstHitMisc[1 + index], firstHitMisc[2 + index]);
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

in vec2 uv;
layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec3 posBuffer;
layout(location = 2) out vec3 normalBuffer;
layout(location = 3) out vec3 miscBuffer;

void main()
{
    ivec3 size = resolution; // imageSize(hitPosition);
    vec3 color = vec3(0);

    vec3 normal = getNormal(ivec3(gl_FragCoord.xy, 0));
    vec3 position = getPosition(ivec3(gl_FragCoord.xy, 0));
    vec3 material = getMisc(ivec3(gl_FragCoord.xy, 0));

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

    for (int i = 0; i < size.z; i++)
    {
        ivec3 texelCoord = ivec3(gl_FragCoord.xy, i);

        vec3 light = getLight(texelCoord);

        vec3 colorBase = light;
        // vec3 colorBase = vec3(material.x);
        //  vec3 colorBase = -vec3(normal.x, normal.y, normal.z) * frameCount;
        //  vec3 colorBase = 0.001 * vec3(position.x, position.y, position.z) * frameCount;

        color += colorBase;
    }
    color /= size.z;

    fragColor = vec4(color, 1);
    posBuffer = position;
    miscBuffer = material;
    normalBuffer = normal;
}
