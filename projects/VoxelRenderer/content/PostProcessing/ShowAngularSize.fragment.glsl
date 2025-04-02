#version 460 core

layout(binding = 0) uniform sampler2D inputTexture;

layout(location = 0) out vec4 out_color;

in vec2 uv;

uniform float horizontalFov;
uniform ivec2 resolution;
uniform float angularSize;

void main()
{
    //if(length((uv - 0.5) * horizontalFov * vec2(1, float(resolution.y) / resolution.x)) < angularSize){
    //    out_color = vec4(1 - texture(inputTexture, uv).xyz, 1);
    //}else{
    //    out_color = vec4(texture(inputTexture, uv));
    //}
    
    if(length((uv - 0.5) * vec2(1, float(resolution.y) / resolution.x)) * horizontalFov < 0.5 * angularSize){
        out_color = vec4(1 - min(vec3(1), texture(inputTexture, uv).xyz), 1);
    }else{
        out_color = vec4(texture(inputTexture, uv));
    }
}
