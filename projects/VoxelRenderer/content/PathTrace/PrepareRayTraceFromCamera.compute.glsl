#version 460 core

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(std430, binding = 0) buffer RayPosition
{
    float rayPosition[];
};

layout(std430, binding = 1) buffer RayDirection
{
    float rayDirection[];
};

layout(std430, binding = 2) buffer PrimaryDirection
{
    writeonly float primaryDirection[];
};


uniform ivec3 resolution; //(xSize, ySize, 1)
uniform vec3 camPosition;
uniform vec4 camRotation;
uniform float horizontalFovTan; // This equals tan(horizontal fov * 0.5)
uniform vec2 jitter; //([0, 1), [0, 1))


void setPrimaryDirection(ivec3 coord, vec3 value){
    int index = 3 * (coord.x + resolution.x * (coord.y)); // Stride of 1, axis order is x y
    primaryDirection[index + 0] = value.x;
    primaryDirection[index + 1] = value.y;
    primaryDirection[index + 2] = value.z;
}


// Applies a quaternion
vec3 qtransform(vec4 q, vec3 v)
{
    return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + q.w * v);
}

// hash and random made by Spatial on 05 July 2013  (someone on stack overflow)
//  A single iteration of Bob Jenkins' One-At-A-Time hashing algorithm.
uint hash(uint x)
{
    x += (x << 10u);
    x ^= (x >> 6u);
    x += (x << 3u);
    x ^= (x >> 11u);
    x += (x << 15u);
    return x;
}

// Compound versions of the hashing algorithm I whipped together.
uint hash(uvec2 v) { return hash(v.x ^ hash(v.y)); }
uint hash(uvec3 v) { return hash(v.x ^ hash(v.y) ^ hash(v.z)); }
uint hash(uvec4 v) { return hash(v.x ^ hash(v.y) ^ hash(v.z) ^ hash(v.w)); }

// Construct a float with half-open range [0:1] using low 23 bits.
// All zeroes yields 0.0, all ones yields the next smallest representable value below 1.0.
float floatConstruct(uint m)
{
    const uint ieeeMantissa = 0x007FFFFFu; // binary32 mantissa bitmask
    const uint ieeeOne = 0x3F800000u; // 1.0 in IEEE binary32

    m &= ieeeMantissa; // Keep only mantissa bits (fractional part)
    m |= ieeeOne; // Add fractional part to 1.0

    float f = uintBitsToFloat(m); // Range [1:2]
    return f - 1.0; // Range [0:1]
}

// Pseudo-random value in half-open range [0:1].
float random(float x) { return floatConstruct(hash(floatBitsToUint(x))); }
float random(vec2 v) { return floatConstruct(hash(floatBitsToUint(v))); }
float random(vec3 v) { return floatConstruct(hash(floatBitsToUint(v))); }
float random(vec4 v) { return floatConstruct(hash(floatBitsToUint(v))); }

void setPos(ivec3 coord, vec3 value)
{
    int index = 3 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride of 3, axis order is x y z
    rayPosition[0 + index] = value.x;
    rayPosition[1 + index] = value.y;
    rayPosition[2 + index] = value.z;
}

void setDir(ivec3 coord, vec3 value)
{
    int index = 3 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride of 3, axis order is x y z
    rayDirection[0 + index] = value.x;
    rayDirection[1 + index] = value.y;
    rayDirection[2 + index] = value.z;
}

vec3 getPos(ivec3 coord)
{
    int index = 3 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride of 3, axis order is x y z
    return vec3(rayPosition[0 + index], rayPosition[1 + index], rayPosition[2 + index]);
}

vec3 getDir(ivec3 coord)
{
    int index = 3 * (coord.x + resolution.x * (coord.y + resolution.y * coord.z)); // Stride of 3, axis order is x y z
    return vec3(rayDirection[0 + index], rayDirection[1 + index], rayDirection[2 + index]);
}

void main()
{
    ivec3 texelCoord = ivec3(gl_GlobalInvocationID.xyz);
    ivec3 size = resolution;

    // Do nothing if the texel coord does not correspond to a valid position in the texture
    if (any(greaterThanEqual(texelCoord, size)))
    {
        return;
    }

    vec3 forward = qtransform(camRotation, vec3(1, 0, 0));
    vec3 up = qtransform(camRotation, vec3(0, 0, 1));
    vec3 right = qtransform(camRotation, vec3(0, -1, 0));

    // The random offset allows for temporal accumulation
    vec2 randOffset = 0 * (vec2(random(vec3(texelCoord) + jitter.x), random(vec3(texelCoord) + jitter.y)) - vec2(0.5)); // Create a random offset using the position of the texel and a provided jitter

    vec2 uv = ((texelCoord.xy + randOffset) / size.xy - 0.5) * 2.0; //([-1, 1), [-1, 1))
    uv.y *= float(size.y) / size.x; // Correct for aspect ratio

    vec3 rayDir = forward + horizontalFovTan * (uv.x * right + uv.y * up);

    setPos(texelCoord, camPosition);
    setDir(texelCoord, rayDir);
    setPrimaryDirection(texelCoord, rayDir);
}
