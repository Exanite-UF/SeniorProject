#version 440


layout(std430, binding = 0) buffer AccumulatedLight
{
    float accumulatedLight[];
};

uniform ivec3 resolution; //(xSize, ySize, raysPerPixel)


vec3 getLight(ivec3 coord)
{
    int index = 3 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride of 3, axis order is x y

    return vec3(accumulatedLight[0 + index], accumulatedLight[1 + index], accumulatedLight[2 + index]);
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

        vec3 light = getLight(texelCoord);

        vec3 colorBase = light;

        color += colorBase;
    }
    color /= size.z;
    fragColor = vec4(color, 1);

    // fragColor = vec4(gl_FragCoord.xy / size.xy, 0, 1);
}
