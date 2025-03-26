#version 460 core

layout(binding = 0) uniform sampler2D inputColor;//tempColorTexture
layout(binding = 1) uniform sampler2D inputPosition;//positionHistoryTexture
layout(binding = 2) uniform sampler2D inputNormal;//normalHistoryTexture
layout(binding = 3) uniform sampler2D inputMisc;//motionInputTexture
layout(binding = 4) uniform sampler2D inputVariance;//motionInputTexture

in vec2 uv;
layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec4 posBuffer;
layout(location = 2) out vec4 normalBuffer;
layout(location = 3) out vec4 miscBuffer;

void main()
{
    fragColor = texture(inputColor, uv);
    posBuffer = texture(inputPosition, uv);
    normalBuffer = texture(inputNormal, uv);
    miscBuffer = texture(inputMisc, uv);
}
