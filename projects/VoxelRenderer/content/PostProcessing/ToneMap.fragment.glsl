#version 460 core

layout(binding = 0) uniform sampler2D inputTexture;

layout(location = 0) out vec4 out_color;

in vec2 uv;

vec3 toneMapFilmic(vec3 color)
{
    color = max(vec3(0.0), color - 0.004);
    return (color * (6.2 * color + 0.5)) / (color * (6.2 * color + 1.7) + 0.06);
}


vec3 toneMapACES(vec3 color)
{
    return clamp((color * (2.51 * color + 0.03)) / (color * (2.43 * color + 0.59) + 0.14), 0.0, 1.0);
}

void main()
{
    vec3 light = texture(inputTexture, uv).xyz;
    // light = normalize(light) * (1 - exp(-length(light)));
    // light = normalize(light) * length(light) / (length(light) + 1);//(1 - exp(-length(light)));
    // light = toneMapFilmic(light);
    light = toneMapACES(light);
    out_color = vec4(light, 1); // vec4(TonemapAces(exposure * texture(inputTexture, uv).xyz), 1);
}
