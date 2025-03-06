#version 460 core

layout(binding = 0) uniform sampler2D inputTexture;
layout(binding = 1) uniform sampler2D positionTexture;
layout(binding = 2) uniform sampler2D normalTexture;
layout(binding = 3) uniform sampler2D materialTexture;

layout(location = 0) out vec4 out_color;

uniform vec3 cameraPosition;
uniform vec4 cameraRotation;
uniform float cameraTanFOV;
uniform ivec2 resolution;

vec3 qtransform(vec4 q, vec3 v)
{
    return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + q.w * v);
}

void main()
{
    vec3 position = texelFetch(positionTexture, ivec2(gl_FragCoord.xy), 0).xyz;
    vec3 normal = texelFetch(normalTexture, ivec2(gl_FragCoord.xy), 0).xyz;
    vec3 material = texelFetch(materialTexture, ivec2(gl_FragCoord.xy), 0).xyz;


    vec4 sum = vec4(0);
    float total = 0;

    int kernelSize = 5;
    for(int i = -kernelSize; i <= kernelSize; i++){
        for(int j = -kernelSize; j <= kernelSize; j++){
            vec3 samplePosition = texelFetch(positionTexture, ivec2(gl_FragCoord.x + i, gl_FragCoord.y + j), 0).xyz;
            vec3 sampleNormal = texelFetch(normalTexture, ivec2(gl_FragCoord.x + i, gl_FragCoord.y + j), 0).xyz;
            vec3 sampleMaterial = texelFetch(materialTexture, ivec2(gl_FragCoord.x + i, gl_FragCoord.y + j), 0).xyz;

            if(!all(equal(normal, sampleNormal))){
                continue;
            }

            if(!all(equal(material, sampleMaterial))){
                continue;
            }

            vec4 color = texelFetch(inputTexture, ivec2(gl_FragCoord.x + i, gl_FragCoord.y + j), 0);

            sum += color;
            total += 1;
        }
    }

    out_color = sum / total;//texelFetch(inputTexture, ivec2(gl_FragCoord.xy), 0);
}
