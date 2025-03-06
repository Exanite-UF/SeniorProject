#version 460 core

layout(binding = 0) uniform sampler2D inputTexture;

layout(location = 0) out vec4 out_color;

uniform bool isXAxis;
uniform int kernelRadius;
uniform float kernel[101];//A maximum radius of 50 pixels

void main()
{
    int temp = int(isXAxis);
    int temp2 = 1 - temp;
    vec4 sum = vec4(0);
    for(int i = -kernelRadius; i <= kernelRadius; i++){
        int j = i + kernelRadius;
        sum += kernel[j] * texelFetch(inputTexture, ivec2(gl_FragCoord.x + i * temp, gl_FragCoord.y + i * temp2), 0);
    }

    out_color = sum;
}
