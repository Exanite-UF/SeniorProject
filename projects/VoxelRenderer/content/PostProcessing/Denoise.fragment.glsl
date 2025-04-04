#version 460 core

layout(binding = 0) uniform sampler2D inputTexture;
layout(binding = 1) uniform sampler2D positionTexture;
layout(binding = 2) uniform sampler2D normalTexture;
layout(binding = 3) uniform sampler2D miscTexture;

layout(location = 0) out vec4 out_color;

uniform bool isXAxis;
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
    int temp = int(isXAxis);
    int temp2 = 1 - temp;

    vec3 position = texelFetch(positionTexture, ivec2(gl_FragCoord.xy), 0).xyz;
    vec3 normal = texelFetch(normalTexture, ivec2(gl_FragCoord.xy), 0).xyz;
    vec3 material = texelFetch(miscTexture, ivec2(gl_FragCoord.xy), 0).xyz;

    vec3 cameraSpacePosition = qtransform(vec4(cameraRotation.xyz, -cameraRotation.w), position - cameraPosition);

    float stdev = material.x * ((cameraSpacePosition.x) * 0.002 + 0.01);

    if (stdev <= 0)
    {
        // This will be true for materials with 0 roughness or the sky box
        out_color = texelFetch(inputTexture, ivec2(gl_FragCoord.x, gl_FragCoord.y), 0);
        return;
    }

    float variance = stdev * stdev;

    float aspectRatio = resolution.x / float(resolution.y);

    // y is parallel with the horizontal of the screen
    vec3 otherVector = (isXAxis) ? vec3(0, 1, 0) : vec3(0, 0, 1);
    float slope = length(dot(normal, otherVector)); // How far do we move for every unit along the chosen axis
    float neededDist = (2 * stdev) / slope; // How far along out chosen axis do we need to move to get to 2 standard deviations away

    vec2 screenDist = ((((cameraSpacePosition + otherVector * neededDist).yz * cameraTanFOV) / abs(cameraSpacePosition.x)) * vec2(-1, aspectRatio) + 1) * 0.5 * resolution.xy - gl_FragCoord.xy;

    int kernelSize = int(ceil(length(screenDist)));

    kernelSize = max(min(10, kernelSize), 0);

    if (kernelSize == 0)
    {
        out_color = texelFetch(inputTexture, ivec2(gl_FragCoord.x, gl_FragCoord.y), 0);
        return;
    }

    vec4 sum = vec4(0);
    float total = 0;
    for (int i = -kernelSize; i <= kernelSize; i++)
    {

        vec3 samplePosition = texelFetch(positionTexture, ivec2(gl_FragCoord.x + i * temp, gl_FragCoord.y + i * temp2), 0).xyz;
        vec3 sampleNormal = texelFetch(normalTexture, ivec2(gl_FragCoord.x + i * temp, gl_FragCoord.y + i * temp2), 0).xyz;
        vec3 sampleMaterial = texelFetch(miscTexture, ivec2(gl_FragCoord.x + i * temp, gl_FragCoord.y + i * temp2), 0).xyz;

        if (sampleMaterial.x != material.x)
        {
            continue;
        }

        if (dot(sampleNormal, normal) < 0.9)
        {
            continue;
        }

        float dist = length(samplePosition - position);
        if (dist > 2 * stdev)
        {
            continue;
        }

        float multiplier = exp(-dist * dist / variance * 0.5);

        vec4 color = texelFetch(inputTexture, ivec2(gl_FragCoord.x + i * temp, gl_FragCoord.y + i * temp2), 0);

        if (any(isnan(color)))
        {
            continue;
        }

        sum += multiplier * color;
        total += multiplier;
    }

    // out_color = vec4(vec3(kernelSize / 10.0), 1);
    out_color = sum / total;
}
