#version 460 core

layout(binding = 0) uniform sampler2D oldColor;
layout(binding = 1) uniform sampler2D miscData;

layout(binding = 2) uniform sampler2D oldPosData;
layout(binding = 3) uniform sampler2D newPosData;

in vec2 uv;

uniform vec3 cameraPosition;
uniform ivec2 resolution;

layout(location = 0) out vec4 out_color;

vec4 safeVec4(vec4 v, vec4 fallback)
{
    return vec4(
        isnan(v.r) ? fallback.r : v.r,
        isnan(v.g) ? fallback.g : v.g,
        isnan(v.b) ? fallback.b : v.b,
        isnan(v.a) ? fallback.a : v.a);
}

void main()
{
    vec3 misc = texture(miscData, uv).xyz;
    vec2 motionVector = misc.yz;
    vec2 oldUV = uv - motionVector;

    vec4 color = safeVec4(texture(oldColor, oldUV), vec4(0)); // bilinearFilter(oldColor, oldUV, resolution);

    vec3 oldPos = texture(oldPosData, oldUV).xyz;
    vec3 newPos = texture(newPosData, uv).xyz;

    float alpha = (misc.x < 0) ? 0 : 0.9;
    if (color.w == 0)
    {
        alpha = 0;
    }

    if (length(oldPos - newPos) / length(newPos - cameraPosition) > 0.1 || length(oldPos - newPos) > 1)
    {
        alpha = 0;
    }

    out_color = vec4(color.xyz, alpha);
}
