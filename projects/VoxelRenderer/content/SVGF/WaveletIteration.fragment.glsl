#version 460 core

layout(binding = 0) uniform sampler2D inputColor;//tempColorTexture
layout(binding = 1) uniform sampler2D inputVariance;//tempVarianceTexture
layout(binding = 2) uniform sampler2D inputDepth;//depthTexture
layout(binding = 3) uniform sampler2D inputNormal;//normalHistoryTexture

in vec2 uv;

uniform ivec2 resolution;

layout(location = 0) out vec4 out_color;//tempColorTexture
layout(location = 1) out vec4 out_variance;//tempVarianceTexture

vec4 safeVec4(vec4 v, vec4 fallback) {
    return vec4(
        isnan(v.r) ? fallback.r : v.r,
        isnan(v.g) ? fallback.g : v.g,
        isnan(v.b) ? fallback.b : v.b,
        isnan(v.a) ? fallback.a : v.a
    );
}

//Outputs the new color
//Set 2 primary output textures
vec3 waveletIteration(sampler2D inputColor, sampler2D inputVariance, sampler2D inputNormal, sampler2D inputDepth){
    vec3 outColor = texture(inputColor, uv).xyz;
    out_color = vec4(outColor, 1);
    out_variance = vec4(texture(inputVariance, uv).xyz, 1);
    return outColor;
}

void main()
{
    waveletIteration(inputColor, inputVariance, inputNormal, inputDepth);//This needs to set the historical color data as well
}
