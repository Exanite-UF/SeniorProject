#version 440

layout(rgba32f, binding = 0) uniform image3D hitPosition;
layout(rgba32f, binding = 1) uniform image3D hitNormal;
layout(r16ui, binding = 2) uniform uimage3D hitMaterial;

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
    ivec3 size = imageSize(hitPosition);

    vec3 color = vec3(0);
    for (int i = 0; i < size.z; i++)
    {
        vec4 pos = imageLoad(hitPosition, ivec3(gl_FragCoord.xy, i));
        vec4 normal = imageLoad(hitNormal, ivec3(gl_FragCoord.xy, i));
        uint material = imageLoad(hitMaterial, ivec3(gl_FragCoord.xy, i)).r;
        float falloff = (normal.w * 0.01 + 1) * (normal.w * 0.01 + 1);

        /*
        uint r = (material & 1) + ((material & 8) >> 2) + ((material & 64) >> 4);
        uint g = ((material & (1 << 1)) >> 1) + ((material & (8 << 1)) >> 3) + ((material & (64 << 1)) >> 5);
        uint b = ((material & (1 << 2)) >> 2) + ((material & (8 << 2)) >> 4) + ((material & (64 << 2)) >> 6);

        vec3 colorBase = vec3(r / 7.0, g / 7.0, b / 7.0);
        */
        
        //This is the workload rendering code
        vec3 colorBase = vec3(material / 100.f);

        if (material > 100)
        {
            int temp = min(200, int(material));
            colorBase = hueToRGB(0.5 - (material - 100) / 200.f);
        }

        color += colorBase;// / falloff;
    }
    color /= size.z;
    fragColor = vec4(color, 1);

    // fragColor = vec4(gl_FragCoord.xy / size.xy, 0, 1);
}
