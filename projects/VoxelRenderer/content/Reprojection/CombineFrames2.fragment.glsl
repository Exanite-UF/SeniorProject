#version 460 core

layout(binding = 0) uniform sampler2D tempColor;
layout(binding = 1) uniform sampler2D colorSquared;
layout(binding = 2) uniform sampler2D miscData;
layout(binding = 3) uniform sampler2D posData;
layout(binding = 4) uniform sampler2D normalData;

in ivec4 gl_FragCoord;
in vec2 uv;

uniform vec3 cameraPosition;
uniform vec4 cameraRotation;
uniform float horizontalFovTan;

uniform ivec2 resolution;

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec4 out_variance;
layout(location = 2) out vec4 out_color_2;

vec4 safeVec4(vec4 v, vec4 fallback) {
    return vec4(
        (isnan(v.r) || isinf(v.r)) ? fallback.r : v.r,
        (isnan(v.g) || isinf(v.g)) ? fallback.g : v.g,
        (isnan(v.b) || isinf(v.b)) ? fallback.b : v.b,
        (isnan(v.a) || isinf(v.a)) ? fallback.a : v.a
    );
}

vec4 bilinearFilter(sampler2D tex, vec2 uv, vec2 texSize) {
    uv = uv * texSize - 0.5; // Convert to texel space

    vec2 uvFloor = floor(uv); // Integer texel coordinates
    vec2 uvFrac = fract(uv); // Fractional part for interpolation

    // Compute texture coordinate offsets
    vec2 uv00 = (uvFloor + vec2(0.0, 0.0) + 0.5) / texSize;
    vec2 uv10 = (uvFloor + vec2(1.0, 0.0) + 0.5) / texSize;
    vec2 uv01 = (uvFloor + vec2(0.0, 1.0) + 0.5) / texSize;
    vec2 uv11 = (uvFloor + vec2(1.0, 1.0) + 0.5) / texSize;

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

vec3 qtransform(vec4 q, vec3 v)
{
    return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + q.w * v);
}
 
vec3 sampleVariance(){
    vec3 result = vec3(0);
    float totalWeight = 0;
    float kernel[3] = float[3](1.0 / 4, 3.0 / 8, 1.0 / 4);

    ivec2 pixelCoord = gl_FragCoord.xy;

    for(int i = 0; i < 3; i++){
        for(int j = 0; j < 3; j++){
            vec2 offset = vec2(i, j) - vec2(1, 1);
            vec2 coord = (pixelCoord + offset + 0.5) / resolution;

            if(any(lessThan(coord, vec2(0))) || any(greaterThan(coord, vec2(1)))){
                continue;
            }

            float multiplier = kernel[i] * kernel[j];
            vec3 color = texture(tempColor, coord).xyz;
            vec3 variance = vec3(length(texture(colorSquared, coord).xyz - (color * color)));
            
            result += variance * multiplier;
            totalWeight += multiplier;
        }
    }

    return result / totalWeight;
}

void main()
{
    //This is where the wavelet filter is applied
    vec3 color = texture(tempColor, uv).xyz;
    vec3 variance = texture(colorSquared, uv).xyz - color * color;

    {
        vec4 outColor = vec4(color, 1);
        //if(any(lessThan(variance, vec3(0)))){
        //    outColor = vec4(0,1, 1, 1);
        //}
        out_color = outColor;
        out_color_2 = outColor;
        return;
    }
    

    vec3 sampledVariance = sampleVariance();

    vec3 position = texture(posData, uv).xyz;
    vec3 relativePosition = qtransform(vec4(cameraRotation.xyz, -cameraRotation.w), position - cameraPosition);

    float depth = relativePosition.x; 
    vec3 normal = texture(normalData, uv).xyz;

    vec2 depthGradient = (normal.x) / -vec2(-normal.y, normal.z);
    depthGradient = (-depthGradient * depth + vec2(-relativePosition.y, relativePosition.z)) / pow((2 * (uv * vec2(1, float(resolution.y) / resolution.x)) - 1) - depthGradient, vec2(2)) * (2.0 / vec2(resolution));

    float paramDepthRejection = 1;
    float paramNormalRejection = 128;
    float paramLuminanceRejection = 4;

    ivec2 pixelCoord = gl_FragCoord.xy;
    float kernel[5] = float[5](1.0 / 16, 1.0 / 4, 3.0 / 8, 1.0 / 4, 1.0 / 16);

    vec3 total = vec3(0);
    vec3 totalWeight = vec3(0);

    vec3 totalVariance = vec3(0);

    for(int i = 0; i < 5; i++){
        for(int j = 0; j < 5; j++){
            vec2 offset = vec2(i, j) - vec2(2, 2);
            vec2 coord = (pixelCoord + offset + 0.5) / resolution;
//
            if(any(lessThan(coord, vec2(0))) || any(greaterThan(coord, vec2(1)))){
                continue;
            }
//
            float multiplier = kernel[i] * kernel[j];
//
            vec3 relativePosition = qtransform(vec4(cameraRotation.xyz, -cameraRotation.w), texture(posData, coord).xyz - cameraPosition);
            float tempDepth = relativePosition.x;
            
            float weightZ = exp(-abs(tempDepth - depth) / (paramDepthRejection * abs(dot(depthGradient, offset)) + 0.000001));
            float weightN = pow(max(0, dot(normal, texture(normalData, coord).xyz)), paramNormalRejection);
            vec3 weightL = exp(-length(color - texture(tempColor, coord).xyz)/(paramLuminanceRejection * sqrt(sampledVariance) + 0.000001));
            vec3 weight = weightZ * weightN * weightL;
//
            vec3 color2 = texture(tempColor, coord).xyz;
//
            //total += abs(abs(tempDepth - depth) - abs(dot(depthGradient, offset))) * multiplier;//color2 * weight * multiplier;
            //totalWeight += multiplier;//weight * multiplier;

            total += color2 * weight * multiplier;
            totalWeight += weight * multiplier;
//
            vec3 tempVariance = texture(colorSquared, coord).xyz - color2 * color2;
            totalVariance += multiplier * multiplier * weight * weight * tempVariance;
        }
    }

    vec4 outColor = safeVec4(vec4(total / totalWeight, 1), vec4(color, 1));
    //if(any(lessThan(variance, vec3(0)))){
    //    outColor = vec4(0,1, 1, 1);
    //}
    out_color = outColor;
    out_color_2 = outColor;
    out_variance = safeVec4(vec4(totalVariance / (totalWeight * totalWeight), 1), vec4(variance, 1));

    //out_color = safeVec4(vec4(totalVariance / (totalWeight * totalWeight), 1), vec4(1,1,0, 1));
}
