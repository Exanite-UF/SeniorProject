#version 460 core

layout(binding = 0) uniform sampler2D inputTexture;
layout(binding = 1) uniform sampler2D positionTexture;
layout(binding = 2) uniform sampler2D normalTexture;
layout(binding = 3) uniform sampler2D miscTexture;

layout(location = 0) out vec4 out_color;

in vec2 uv;

uniform int whichTexture;

void main()
{
    switch(whichTexture){
        case 0:
            out_color = texture(inputTexture, uv);
            break;
        case 1:
            out_color = texture(positionTexture, uv) / 512.0;
            break;
        case 2:
            out_color = texture(normalTexture, uv);
            break;
        case 3:
            out_color = texture(miscTexture, uv);
            break;
        default:
            out_color = texture(inputTexture, uv);
            break;
    }

    //out_color = vec4(1);
}
