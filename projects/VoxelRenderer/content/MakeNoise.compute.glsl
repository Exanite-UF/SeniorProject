#version 440 core

layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;

layout(r8ui, binding = 0) uniform uimage3D imgOutput;
uniform float time;
uniform bool isRand2;
uniform float fillAmount;

float rand(vec3 x)
{
    return fract(sin(dot(x, vec3(12.9898, 78.233, 21.6452))) * 43758.5453 + time);
}

float wave(float x)
{
    return sin(x) * 0.5 + 0.5;
}

float rand2(vec3 x)
{
    float result = 0;
    result += wave(0.0173651103013 * dot(x, vec3(0.587850564235, 0.758284772211, 0.43462848748)) + time);
    result += wave(0.0685989677893 * dot(x, vec3(-0.376693290124, 0.0670154499555, 0.0673054752805)) + time);
    result += wave(0.0117508870384 * dot(x, vec3(0.2477844726, 0.412264222346, 0.561235509959)) + time);
    result += wave(0.05551583823 * dot(x, vec3(0.280613371903, -0.566017829842, 0.581708656338)) + time);
    return result / 4;
}

void main()
{

    ivec3 texelCoord = ivec3(gl_GlobalInvocationID.xyz);

    vec3 pos = 2 * texelCoord;

    uint final = 0;
    if (isRand2)
    {
        int k = 1;
        for (int i = 0; i < 2; i++)
        {
            for (int j = 0; j < 2; j++)
            {
                for (int l = 0; l < 2; l++)
                {
                    float value = 0;
                    vec3 temp = pos + vec3(l, j, i);
                    value = rand2(temp);
                    value += rand2(floor(temp / 2 + 1));
                    value += rand2(floor(temp / 4 + 2));
                    value += rand2(floor(temp / 8 + 3));
                    value /= 4;
                    if (value > fillAmount)
                    { // This is how you set the density
                        final |= k;
                    }
                    k = k << 1;
                }
            }
        }
    }
    else
    {
        int k = 1;
        for (int i = 0; i < 2; i++)
        {
            for (int j = 0; j < 2; j++)
            {
                for (int l = 0; l < 2; l++)
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
    }

    imageStore(imgOutput, texelCoord, uvec4(final));
}
