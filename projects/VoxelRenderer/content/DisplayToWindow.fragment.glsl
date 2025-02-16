#version 440

layout(std430, binding = 0) buffer HitPosition
{
    vec4 hitPosition[];
};
layout(std430, binding = 1) buffer HitNormal
{
    vec4 hitNormal[];
};

layout(std430, binding = 2) buffer HitMaterial
{
    uint hitMaterial[];
};

layout(std430, binding = 3) buffer HitVoxelPosition
{
    float hitVoxelPosition[];
};




uniform ivec3 resolution; //(xSize, ySize, raysPerPixel)


vec4 getHitPosition(ivec3 coord){
    int index = (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // axis order is x y z

    return hitPosition[index];
}

vec4 getHitNormal(ivec3 coord){
    int index = (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // axis order is x y z

    return hitNormal[index];
}

uint getHitMaterial(ivec3 coord){
    int index = (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // axis order is x y z

    return hitMaterial[index];
}

vec3 getHitVoxelPosition(ivec3 coord)
{
    int index = 3 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride of 3, axis order is x y 
    
    return vec3(hitVoxelPosition[0 + index], hitVoxelPosition[1 + index], hitVoxelPosition[2 + index]);
}

out vec4 fragColor;

vec3 hueToRGB(float hue)
{
    hue = mod(hue, 1.0);
    float r = abs(hue * 6.0 - 3.0) - 1.0;
    float g = 2.0 - abs(hue * 6.0 - 2.0);
    float b = 2.0 - abs(hue * 6.0 - 4.0);
    return clamp(vec3(r, g, b), 0.0, 1.0);
}

void main()
{
    ivec3 size = resolution;//imageSize(hitPosition);

    vec3 color = vec3(0);
    for (int i = 0; i < size.z; i++)
    {
        ivec3 texelCoord = ivec3(gl_FragCoord.xy, i);
        vec4 pos = getHitPosition(texelCoord);//imageLoad(hitPosition, ivec3(gl_FragCoord.xy, i));
        vec4 normal = getHitNormal(texelCoord);//imageLoad(hitNormal, ivec3(gl_FragCoord.xy, i));
        uint material = getHitMaterial(texelCoord);//imageLoad(hitMaterial, ivec3(gl_FragCoord.xy, i)).r;
        vec3 voxelPos = getHitVoxelPosition(texelCoord);
        float falloff = (normal.w * 0.01 + 1) * (normal.w * 0.01 + 1);

        // This is the pseudo material rendering code
        uint r = (material & 1) + ((material & 16) >> 3) + ((material & 256) >> 6);
        uint g = ((material & (1 << 1)) >> 1) + ((material & (16 << 1)) >> 4) + ((material & (256 << 1)) >> 7);
        uint b = ((material & (1 << 2)) >> 2) + ((material & (16 << 2)) >> 5) + ((material & (256 << 2)) >> 8);

        //vec3 colorBase = vec3(r / 7.0, g / 7.0, b / 7.0);
        vec3 voxelPosition = getHitVoxelPosition(texelCoord);
        vec2 hitUV;
        if(abs(normal.x) > 0){
            //yz
            hitUV = voxelPosition.yz;
        }else if(abs(normal.y) > 0){
            //xz
            hitUV = voxelPosition.xz;
        }else if(abs(normal.z) > 0){
            //xy
            hitUV = voxelPosition.xy;
        }

        hitUV *= 0.1;
        hitUV = mod(hitUV, 1);

        vec3 colorBase = vec3(hitUV, 0);

        /*
        // This is the workload rendering code
        vec3 colorBase = vec3(material / 100.f);

        if (material > 100)
        {
            int temp = min(200, int(material));
            colorBase = hueToRGB(0.5 - (material - 100) / 200.f);
        }
        */

        // vec3 colorBase = abs(normal.xyz);
        color += colorBase / falloff;
    }
    color /= size.z;
    fragColor = vec4(color, 1);

    // fragColor = vec4(gl_FragCoord.xy / size.xy, 0, 1);
}
