#version 460 core

layout(binding = 0) uniform sampler2D oldColor;
layout(binding = 1) uniform sampler2D newColor;
layout(binding = 2) uniform sampler2D oldColorSquared;
layout(binding = 3) uniform sampler2D miscData;

layout(binding = 4) uniform sampler2D oldPosData;
layout(binding = 5) uniform sampler2D newPosData;

in vec2 uv;

uniform ivec2 resolution;

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec4 out_color_squared;

vec4 safeVec4(vec4 v, vec4 fallback) {
    return vec4(
        isnan(v.r) ? fallback.r : v.r,
        isnan(v.g) ? fallback.g : v.g,
        isnan(v.b) ? fallback.b : v.b,
        isnan(v.a) ? fallback.a : v.a
    );
}

vec4 bilinearFilter(sampler2D tex, vec2 uv, vec2 texSize) {
    uv = uv * texSize; // Convert to texel space

    vec2 uvFloor = floor(uv); // Integer texel coordinates
    vec2 uvFrac = fract(uv); // Fractional part for interpolation

    // Compute texture coordinate offsets
    vec2 uv00 = (uvFloor + vec2(0.0, 0.0) + 0 * 0.5) / texSize;
    vec2 uv10 = (uvFloor + vec2(1.0, 0.0) + 0 * 0.5) / texSize;
    vec2 uv01 = (uvFloor + vec2(0.0, 1.0) + 0 * 0.5) / texSize;
    vec2 uv11 = (uvFloor + vec2(1.0, 1.0) + 0 * 0.5) / texSize;

    // Sample four nearest texels
    vec4 c00 = texture(tex, uv00);
    vec4 c10 = texture(tex, uv10);
    vec4 c01 = texture(tex, uv01);
    vec4 c11 = texture(tex, uv11);

    c00 = safeVec4(c00, vec4(0));
    c10 = safeVec4(c10, c00);
    c01 = safeVec4(c01, c00);
    c11 = safeVec4(c11, c00);

    // Perform bilinear interpolation
    vec4 c0 = mix(c00, c10, uvFrac.x);
    vec4 c1 = mix(c01, c11, uvFrac.x);
    return mix(c0, c1, uvFrac.y);
}

void main()
{
    vec3 misc = texture(miscData, uv).xyz;
    vec2 motionVector = misc.yz;
    vec2 oldUV = uv - motionVector;

    

    vec4 color = safeVec4(texture(oldColor, oldUV), vec4(0));//bilinearFilter(oldColor, oldUV, resolution);

    vec3 oldPos = texture(oldPosData, oldUV).xyz;
    vec3 newPos = texture(newPosData, uv).xyz;

    float alpha = (misc.x < 0) ? 0 : 0.9;
    if(color.w == 0){
        alpha = 0;
    }

    if(length(oldPos - newPos) > 1){
        alpha = 0;
    }

    vec3 newSample = safeVec4(texture(newColor, uv), vec4(0)).xyz;
    out_color = vec4(color.xyz * alpha + (1 - alpha) * newSample, 1);

    vec4 oldSquared = safeVec4(texture(oldColorSquared, oldUV), vec4(0));//bilinearFilter(oldColorSquared, oldUV, resolution);
    out_color_squared = vec4(oldSquared.xyz * alpha + (1 - alpha) * newSample * newSample, 1);
}
