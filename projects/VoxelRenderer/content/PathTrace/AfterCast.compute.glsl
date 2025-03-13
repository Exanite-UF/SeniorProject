#version 460 core

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;


uniform ivec3 resolution; //(xSize, ySize, raysPerPixel)
uniform bool currentBuffer;

layout(std430, binding = 0) buffer HitMisc
{
    float hitMisc[];
};

void setHitWasHit(ivec3 coord, bool value)
{
    int index = 2 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // axis order is x y z
    hitMisc[index + 0] = (value) ? 1.0 : 0.0;
}

bool getHitWasHit(ivec3 coord)
{
    int index = 2 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // axis order is x y z
    return hitMisc[index + 0] > 0;
}

void setHitDist(ivec3 coord, float value)
{
    int index = 2 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // axis order is x y z
    hitMisc[index + 1] = value;
}

layout(std430, binding = 1) buffer PriorAttenuation1
{
    float priorAttenuation1[];
};

layout(std430, binding = 2) buffer PriorAttenuation2
{
    float priorAttenuation2[];
};



void setAttenuation(ivec3 coord, vec3 value)
{
    int index = 3 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride is 3, axis order is x y z

    if(currentBuffer){
        priorAttenuation1[0 + index] = value.x;
        priorAttenuation1[1 + index] = value.y;
        priorAttenuation1[2 + index] = value.z;
    }else{
        priorAttenuation2[0 + index] = value.x;
        priorAttenuation2[1 + index] = value.y;
        priorAttenuation2[2 + index] = value.z;
    }

}



void main()
{
    ivec3 texelCoord = ivec3(gl_GlobalInvocationID.xyz);

   
    if(!getHitWasHit(texelCoord)){
        setAttenuation(texelCoord, vec3(0));
        setHitDist(texelCoord, -1.0);
    }else{
        setHitDist(texelCoord, 1.0 / 0.0);
    }

    setHitWasHit(texelCoord, false);
}