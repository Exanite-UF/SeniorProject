#version 440 core

layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;

layout(rgba8ui, binding = 0) uniform uimage3D imgOutput;
uniform float time;
uniform float fillAmount;

float rand(vec3 x)
{
    return fract(sin(dot(x, vec3(12.9898, 78.233, 21.6452))) * 43758.5453 + time);
}

void main()
{

    ivec3 texelCoord = ivec3(gl_GlobalInvocationID.xyz);

    vec3 pos = 2 * texelCoord;

    uint final = 0;
    int k = 1;
    for (int i = 0; i < 2; i++) // z axis
    {
        for (int j = 0; j < 2; j++) // y axis
        {
            for (int l = 0; l < 2; l++) // x axis
            {
                float value = 0;
                vec3 temp = pos + vec3(l, j, i);
                value = rand(temp);
                value += rand(floor(temp / 2 + 1));
                value += rand(floor(temp / 4 + 2));
                value += rand(floor(temp / 8 + 3));
                value /= 4;
                if (value > fillAmount)
                { // This is how you set the density
                    final |= k;
                }
                k = k << 1;
            }
        }
    }

    uvec3 material = imageLoad(imgOutput, texelCoord).rgb;

    imageStore(imgOutput, texelCoord, uvec4(material, final));
}
