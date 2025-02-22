#version 440

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

uniform ivec3 resolution; //(xSize, ySize, raysPerPixel)
uniform int frameCount;
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

vec3 qtransform(vec4 q, vec3 v)
{
    return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + q.w * v);
}

out vec4 fragColor;

vec3 hueToRGB(float hue)
{
    hue = mod(hue, 1.0);
    float r = abs(hue * 6.0 - 3.0) - 1.0;
    float g = 2.0 - abs(hue * 6.0 - 2.0);
    float b = 2.0 - abs(hue * 6.0 - 4.0);
    return clamp(vec3(r, g, b), 0.0, 1.0);
}

void main()
{
    ivec3 size = resolution; // imageSize(hitPosition);

    vec3 color = vec3(0);
    for (int i = 0; i < size.z; i++)
    {
        ivec3 texelCoord = ivec3(gl_FragCoord.xy, i);

        vec3 normal = getNormal(texelCoord);
        vec3 position = getPosition(texelCoord);

        normal = qtransform(vec4(-cameraRotation.xyz, cameraRotation.w), normal);
        // normal is now in camera space
        //(1, 0, 0) toward the camera
        //(0, 1, 0) to the right
        //(0, 0, 1) down

        // Put this into a frame buffer (an actual framebuffer)
        // And apply an anisotropic blur using the normal

        position = qtransform(vec4(-cameraRotation.xyz, cameraRotation.w), position - cameraPosition);
        // At this point position is in camera space
        //+x is in front of the camera
        //+y is to the left of the camera
        //+z is above the camera

        vec3 light = getLight(texelCoord);

        vec3 colorBase = light;
        // vec3 colorBase = -vec3(normal.x, normal.y, normal.z) * frameCount;
        // vec3 colorBase = 0.001 * vec3(position.x, position.y, position.z) * frameCount;

        color += colorBase;
    }
    color /= size.z * frameCount;
    fragColor = vec4(color, 1);

    // fragColor = vec4(gl_FragCoord.xy / size.xy, 0, 1);
}
