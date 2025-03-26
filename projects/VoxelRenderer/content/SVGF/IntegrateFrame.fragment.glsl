#version 460 core

layout(binding = 0) uniform sampler2D inputColor;
layout(binding = 1) uniform sampler2D inputMotion;
layout(binding = 2) uniform sampler2D inputPosition;

layout(binding = 3) uniform sampler2D historicalColor;
layout(binding = 4) uniform sampler2D historicalPosition;
layout(binding = 5) uniform sampler2D historicalMoment1;//moment 1 texture
layout(binding = 6) uniform sampler2D historicalMoment2;//moment 2 texture

in vec2 uv;

uniform vec3 cameraPosition;
uniform vec4 cameraRotation;
uniform ivec2 resolution;

layout(location = 0) out vec4 out_color;//tempColorTexture
layout(location = 1) out vec4 out_moment_1;//moment 1 texture
layout(location = 2) out vec4 out_moment_2;//moment 2 texture
layout(location = 3) out vec4 out_variance;//tempVarianceTexture
layout(location = 4) out vec4 out_clip_position;//clipPositionTexture

vec4 safeVec4(vec4 v, vec4 fallback) {
    return vec4(
        isnan(v.r) ? fallback.r : v.r,
        isnan(v.g) ? fallback.g : v.g,
        isnan(v.b) ? fallback.b : v.b,
        isnan(v.a) ? fallback.a : v.a
    );
}

vec3 qtransform(vec4 q, vec3 v)
{
    return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + q.w * v);
}


void main()
{
    vec3 misc = texture(inputMotion, uv).xyz;
    vec2 motionVector = misc.yz;
    vec2 oldUV = uv - motionVector;

    vec4 oldColor = safeVec4(texture(historicalColor, oldUV), vec4(0));
    vec4 oldMoment1 = safeVec4(texture(historicalMoment1, oldUV), vec4(0));
    vec4 oldMoment2 = safeVec4(texture(historicalMoment2, oldUV), vec4(0));

    vec3 oldPos = texture(historicalPosition, oldUV).xyz;
    vec3 newPos = texture(inputPosition, uv).xyz;

    //Alpha is how much of the old data to carry into the rolling average
    float alpha = (misc.x < 0) ? 0 : 0.8;//If the material has a negative roughness then it is part of the skybox
    if(oldColor.w == 0){//If the old color value failed to sample, then do not use the old data
        alpha = 0;
    }

    //Reject old data that comes from a difference location
    //if(length(oldPos - newPos) / length(newPos - cameraPosition) > 0.1 || length(oldPos - newPos) > 1){
    //    alpha = 0;
    //}

    if(length(oldPos - newPos) > 1){
        alpha = 0;
    }

    vec3 newColor = safeVec4(texture(inputColor, uv), vec4(0)).xyz;//Get the incoming color

    //Set the outputs
    out_color = vec4(oldColor.xyz * alpha + (1 - alpha) * newColor, 1);//the color history is filtered

    vec3 newMoment1 = oldMoment1.xyz * alpha + (1 - alpha) * newColor;
    vec3 newMoment2 = oldMoment2.xyz * alpha + (1 - alpha) * newColor * newColor;

    out_moment_1 = vec4(newMoment1, 1);//The moment 1 history is not filtered
    out_moment_2 = vec4(newMoment2, 1);//Moment 2 is the average square of the color

    out_variance = vec4(newMoment2 - newMoment1 * newMoment1, 1);

    //This is the position in clip space
    out_clip_position = vec4(qtransform(vec4(cameraRotation.xyz, -cameraRotation.w), newPos - cameraPosition), 1);
}
