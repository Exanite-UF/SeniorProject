#version 460 core

layout(binding = 0) uniform sampler2D positionTexture;
layout(binding = 1) uniform sampler2D normalTexture;


in vec2 uv;
in float isSkyBox;
in vec3 color;
in vec3 normal;
in vec4 misc;
in vec3 posOut;
in float distance1;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec3 posBuffer; // Global space
layout(location = 2) out vec3 normalBuffer; // Camera space
layout(location = 3) out vec4 miscBuffer;

void main()
{
    float alpha = (isSkyBox > 0 && isSkyBox < 0.99) ? 0 : 1;

    vec3 pos = texture(positionTexture, uv).xyz;

    float temp = length(pos - posOut) / distance1;
    if(temp > 0.1 && isSkyBox == 0){
        alpha = 0;
    }


    //fragColor = vec4(vec3(alpha), 1);
    fragColor = vec4(color * alpha, 1);
    posBuffer = pos;
    normalBuffer = normal;
    miscBuffer = misc;
}
