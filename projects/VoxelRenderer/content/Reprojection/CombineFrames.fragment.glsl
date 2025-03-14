#version 460 core

layout(binding = 0) uniform sampler2D oldPosition;
layout(binding = 2) uniform sampler2D oldColor;

layout(binding = 3) uniform sampler2D newPosition;
layout(binding = 4) uniform sampler2D newMaterial;

layout(binding = 5) uniform sampler2D frameCount;
layout(binding = 6) uniform sampler2D combineMask;

layout(binding = 7) uniform sampler2D oldMaterial;

in vec2 uv;
in float distance1;
in float isSkybox;

uniform vec3 cameraMovement;
uniform ivec2 resolution;

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec4 frameCountOut;

void main()
{
    float frameCount = texture(frameCount, uv).x;

    vec2 localUV = gl_FragCoord.xy / resolution.xy;
    vec3 oldPos = texture(oldPosition, uv).xyz;
    vec3 newPos = texture(newPosition, localUV).xyz;
    float dist = length(newPos - oldPos);
    //frameCount /= 20 * (dist / distance1) + 1; // Distance traveled / distance from camera

    float mask = texture(combineMask, localUV).x;
    frameCount *= mask;

    vec3 material = texture(newMaterial, localUV).xyz;
    vec3 oldMaterial = texture(oldMaterial, uv).xyz;

    if(material.x < 0){
        frameCount = 0;
        material.x = 1;
    }

    float angleChange = atan(length(cameraMovement), distance1);
    float a = 1000000 * 3.1415926589 * pow(1 - material.x, 15) + 1;

    // float temp = -pow(a, -1.57079632679);

    if(abs(oldMaterial.x - material.x) > 0.1){
        frameCount *= 0;
    }
    frameCount *= exp(-10 * angleChange * (1 - material.x * 0.9));

    //frameCount *= pow(a, -angleChange); //(pow(a, -angleChange) + temp) / (1 + temp);
    //frameCount *= 1 - 0.2 * (1 - exp(-length(cameraMovement)));
    //frameCount = min(frameCount, 100);

    out_color = vec4(texture(oldColor, uv).xyz, float(frameCount) / (frameCount + 1));
    //out_color = vec4(vec3(float(frameCount) / (frameCount + 1)), 1);
    //out_color = vec4(vec3(mask), 1);
    out_color = vec4(vec3(material.x, 0 * oldMaterial.x * mask, mask), 1);
    frameCountOut = vec4(frameCount + 1, 0, 0, 1);
}
