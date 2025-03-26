#version 460 core

layout(binding = 0) uniform sampler2D inputColor;//tempColorTexture
layout(binding = 1) uniform sampler2D inputVariance;//tempVarianceTexture
layout(binding = 2) uniform sampler2D inputClipPosition;//clipPositionTexture
layout(binding = 3) uniform sampler2D inputNormal;//normalInputTexture
layout(binding = 4) uniform sampler2D inputMotion;//motionInputTexture
layout(binding = 5) uniform sampler2D inputPosition;//positionInputTexture



in vec2 uv;

uniform float cameraFovTan;
uniform ivec2 resolution;

layout(location = 0) out vec4 out_color;//tempColorTexture
layout(location = 1) out vec4 out_variance;//tempVarianceTexture

layout(location = 2) out vec4 out_color_2;//colorHistoryTexture
layout(location = 3) out vec4 out_normal;//normalHistoryTexture
layout(location = 4) out vec4 out_position;//positionHistoryTexture

vec4 safeVec4(vec4 v, vec4 fallback) {
    return vec4(
        isnan(v.r) ? fallback.r : v.r,
        isnan(v.g) ? fallback.g : v.g,
        isnan(v.b) ? fallback.b : v.b,
        isnan(v.a) ? fallback.a : v.a
    );
}

vec3 hueToRGB(float hue)
{
    hue = mod(hue, 1.0);
    float r = abs(hue * 6.0 - 3.0) - 1.0;
    float g = 2.0 - abs(hue * 6.0 - 2.0);
    float b = 2.0 - abs(hue * 6.0 - 4.0);
    return clamp(vec3(r, g, b), 0.0, 1.0);
}

const vec3 luminanceVector = vec3(0.2126, 0.7152, 0.0722);

//samples a 3x3 area around the pixel for the luminace variance
float getLuminanceVariance(){
    float result = 0;
    float total = 0;
    const float kernel[3] = float[3](1.0 / 4.0, 4.0 / 8.0, 1.0 / 4.0);

    for(int i = 0; i < 3; i++){
        for(int j = 0; j < 3; j++){
            vec2 coord = (uv * resolution + vec2(i - 1, j - 1)) / resolution;
            float multiplier = kernel[i] * kernel[j];
            float temp = dot(texture(inputVariance, coord).xyz, luminanceVector);
            if(!isnan(temp)){
                result += temp * multiplier;
                total += multiplier;
            }
        }
    }

    if(total == 0){
        return dot(texture(inputVariance, uv).xyz, luminanceVector);
    }
    return result / total;
}

const float paramDepthRejection = 1;
const float paramNormalRejection = 128;
const float paramLuminanceRejection = 4;
const float paramRoughnessRejection = 20;

//Outputs the new color
//Set 2 primary output textures
vec3 waveletIteration(sampler2D inputColor, sampler2D inputVariance, sampler2D inputNormal, sampler2D inputClipPosition){
    vec3 normal = texture(inputNormal, uv).xyz;
    vec3 clipPosition = texture(inputClipPosition, uv).xyz;
    vec2 aspectRatio = vec2(1, float(resolution.y) / resolution.x);
    vec3 color = texture(inputColor, uv).xyz;
    vec2 tempMaterial = texture(inputMotion, uv).xw;
    vec4 material = vec4(tempMaterial.x, hueToRGB(tempMaterial.y));

    vec2 depthGradient = normal.x / vec2(-normal.y, normal.z);
    depthGradient = -(depthGradient * clipPosition.x + vec2(-clipPosition.y, clipPosition.z)) * (cameraFovTan * aspectRatio) / pow((cameraFovTan * aspectRatio) * (2 * uv - 1) + depthGradient, vec2(2));
    depthGradient *= 2.0 / vec2(resolution);//Put into a per pixel basis

    float luminanceVariance = getLuminanceVariance();

    const float kernel[5] = float[5](1.0 / 16.0, 1.0 / 4.0, 3.0 / 8.0, 1.0 / 4.0, 1.0 / 16.0);
    
    vec3 colorSum = vec3(0);
    vec3 varianceSum = vec3(0);
    float total = 0;

    for(int i = 0; i < 5; i++){
        for(int j = 0; j < 5; j++){
            vec2 offset = vec2(i - 2, j - 2);
            vec2 coord = (uv * resolution + offset) / resolution;

            vec3 otherClipPosition = texture(inputClipPosition, coord).xyz;
            vec3 otherNormal = texture(inputNormal, coord).xyz;
            vec3 otherColor = texture(inputColor, coord).xyz;
            vec3 otherVariance = texture(inputVariance, coord).xyz;
            tempMaterial = texture(inputMotion, coord).xw;
            vec4 otherMaterial = vec4(tempMaterial.x, hueToRGB(tempMaterial.y));

            float weightZ = exp(-abs(otherClipPosition.x - clipPosition.x) / (paramDepthRejection * abs(dot(depthGradient, offset)) + 0.000001));
            float weightN = pow(max(0, dot(normal, otherNormal)), paramNormalRejection);
            float weightL = exp(-abs(dot(otherColor, luminanceVector) - dot(color, luminanceVector)) / (paramLuminanceRejection * sqrt(luminanceVariance) + 0.000001));
            float weightM = exp(-paramRoughnessRejection * length(material - otherMaterial));
            float weight = weightZ * weightN * weightL * weightM;

            float multiplier = kernel[i] * kernel[j];

            colorSum += multiplier * weight * otherColor;
            varianceSum += multiplier * weight * multiplier * weight * otherVariance;
            total += multiplier * weight;
        }
    }

    vec3 outColor = color;
    vec3 initialVariance = texture(inputVariance, uv).xyz;
    vec3 outVariance = initialVariance;
    if(total > 0){
        outColor = colorSum / total;
        outVariance = varianceSum / (total * total);
    }

    out_color = safeVec4(vec4(outColor, 1), vec4(color, 1));
    out_variance = safeVec4(vec4(outVariance, 1), vec4(initialVariance, 1));
    return outColor;
}

void main()
{
    //These still need to be passed through
    out_normal = texture(inputNormal, uv);
    out_position = texture(inputPosition, uv);


    out_color_2 = vec4(waveletIteration(inputColor, inputVariance, inputNormal, inputClipPosition), 1);//This needs to set the historical color data as well
}
