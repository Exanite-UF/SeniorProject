#version 440

uniform vec2 resolution;
layout(rgba8ui, binding = 0) uniform readonly uimage3D texture1;
layout(rgba8ui, binding = 1) uniform readonly uimage3D texture2;
layout(rgba8ui, binding = 2) uniform readonly uimage3D texture3;
layout(rgba8ui, binding = 3) uniform readonly uimage3D texture4;
layout(rgba8ui, binding = 4) uniform readonly uimage3D texture5;

uniform vec3 camPos;
uniform vec3 camDir;
uniform bool isWorkload;

out vec4 fragColor;

struct RayHit
{
    bool wasHit;
    vec3 hitLocation;
    vec3 normal;
    float dist;
    int iterations;
    uint material;
};

// source: https://gamedev.stackexchange.com/questions/18436/most-efficient-aabb-vs-ray-collision-algorithms
float rayboxintersect(vec3 raypos, vec3 raydir, vec3 boxmin, vec3 boxmax)
{
    if (all(greaterThan(raypos, boxmin)) && all(lessThan(raypos, boxmax)))
    { // ray is inside the box
        return 0;
    }

    float t1 = (boxmin.x - raypos.x) / raydir.x;
    float t2 = (boxmax.x - raypos.x) / raydir.x;
    float t3 = (boxmin.y - raypos.y) / raydir.y;
    float t4 = (boxmax.y - raypos.y) / raydir.y;
    float t5 = (boxmin.z - raypos.z) / raydir.z;
    float t6 = (boxmax.z - raypos.z) / raydir.z;

    float tmin = max(max(min(t1, t2), min(t3, t4)), min(t5, t6));
    float tmax = min(min(max(t1, t2), max(t3, t4)), max(t5, t6));

    if (tmax < 0.0) // box on ray but behind ray origin
    {
        return -1;
    }

    if (tmin > tmax) // ray doesn't intersect box
    {
        return -1;
    }

    return tmin;
}

// Minimal aliasing (Fast)
RayHit findIntersection2(vec3 rayPos, vec3 rayDir)
{
    RayHit hit;
    hit.material = 0;
    hit.wasHit = false;

    rayDir /= length(rayDir);

    vec3 aRayDir = 1 / abs(rayDir); // This is a constant that is used several times
    ivec3 sRayDir = ivec3(1.5 * rayDir / abs(rayDir)); // This is the sign of the ray direction (1.5 is for numerical stability)
    vec3 iRayDir = 1 / rayDir;

    ivec3 size = 2 * imageSize(texture1); // This is the size of the voxel volume

    vec3 rayStart = rayPos;

    // Put the ray at the surface of the cube
    float distToCube = rayboxintersect(rayStart, rayDir, vec3(0), vec3(size));
    rayPos += rayDir * (distToCube - 1);//The -1 is for numerical stability, but it requires the near clipping sphere of 1 unit away

    // If the ray never entered the cube, then quit
    if (distToCube < 0)
    {
        return hit;
    }

    const int iterations = 200;
    bool isOutside = false;

    for (int i = 0; i < iterations; i++)
    {
        hit.iterations = i;
        ivec3 p = ivec3(floor(rayPos)); // voxel coordinate

        vec3 t = ceil(rayPos * sRayDir) * aRayDir - rayPos * iRayDir;
        t += vec3(lessThanEqual(t, vec3(0))) * aRayDir; // Numerical stability correction

        // Stop iterating if you leave the cube that all the voxels are in (1 unit of padding is provided to help with numerical stability)
        if (i > 1 && (any(greaterThan(p, ivec3(size - 1))) || any(lessThan(p, ivec3(0)))))
        {
            hit.normal = vec3(float(i) / iterations);
            // hit.wasHit = true;
            break;
        }

        ivec3 p2 = p & 1; // This lets us disambiguate between the 8 voxels in a cell
        uint k1 = ((1 << p2.x) << (p2.y << 1)) << (p2.z << 2); // This creates the mask that will extract the single bit that we want

        p2 = (p >> 2) & 1; // This lets us disambiguate between the 8 voxels in a cell of level 2
        uint k2 = ((1 << p2.x) << (p2.y << 1)) << (p2.z << 2); // This creates the mask that will extract the single bit that we want

        p2 = (p >> 4) & 1; // This lets us disambiguate between the 8 voxels in a cell of level 3
        uint k3 = ((1 << p2.x) << (p2.y << 1)) << (p2.z << 2); // This creates the mask that will extract the single bit that we want

        p2 = (p >> 6) & 1; // This lets us disambiguate between the 8 voxels in a cell of level 4
        uint k4 = ((1 << p2.x) << (p2.y << 1)) << (p2.z << 2); // This creates the mask that will extract the single bit that we want

        p2 = (p >> 8) & 1; // This lets us disambiguate between the 8 voxels in a cell of level 5
        uint k5 = ((1 << p2.x) << (p2.y << 1)) << (p2.z << 2); // This creates the mask that will extract the single bit that we want

        uvec4 l1 = imageLoad(texture1, (p >> 1));
        uvec4 l2 = imageLoad(texture2, (p >> 3));
        uvec4 l3 = imageLoad(texture3, (p >> 5));
        uvec4 l4 = imageLoad(texture4, (p >> 7));
        uvec4 l5 = imageLoad(texture5, (p >> 9));


        uint level1 = l1.a; // This is the cell from the image (Warning the upper 24 bits are garbage and should be ignored)

        uint level2 = l2.a; // cell for level 2

        uint level3 = l3.a; // cell for level 3

        uint level4 = l4.a; // cell for level 4

        uint level5 = l5.a; // cell for level 4

        // This is the number of mip map levels at which no voxels are found
        int count = int(level5 == 0) + int((level5 & k5) == 0) + int(level4 == 0) + int((level4 & k4) == 0) + int(level3 == 0) + int((level3 & k3) == 0) + int(level2 == 0) + int((level2 & k2) == 0) + int(level1 == 0) + int((level1 & k1) == 0);
        // int count = int(level4 == 0) + int((level4 & k4) == 0) + int(level3 == 0) + int((level3 & k3) == 0) + int(level2 == 0) + int((level2 & k2) == 0) + int(level1 == 0) + int((level1 & k) == 0);
        // int count = int(level3 == 0) + int((level3 & k3) == 0) + int(level2 == 0) + int((level2 & k2) == 0) + int(level1 == 0) + int((level1 & k) == 0);
        // int count = int(level2 == 0) + int((level2 & k2) == 0) + int(level1 == 0) + int((level1 & k) == 0);//This is the number of mip map levels at which no voxels are found
        // int count = int((level1 & k) == 0);//This is the number of mip map levels at which no voxels are found

        if (count <= 0)
        {
            // This means that there was a hit
            if (i > 0 && isOutside)
            { // Don't intersect with the first voxel
                hit.wasHit = true;
                uvec3 c1 = uvec3(greaterThan(l1.rgb & k1, uvec3(0)));//uvec3((l1.r & k1) > 0 ?  1 : 0, (l1.g & k1) > 0 ? 1 : 0, (l1.b & k1) > 0 ? 1 : 0);
                uvec3 c2 = uvec3(greaterThan(l2.rgb & k2, uvec3(0)));//uvec3(l2.r & k2 > 0, l2.g & k2 > 0, l2.b & k2 > 0);
                uvec3 c3 = uvec3(greaterThan(l3.rgb & k3, uvec3(0)));//uvec3(l3.r & k3 > 0, l3.g & k3 > 0, l3.b & k3 > 0);
                hit.material = c1.r + (1 << 1) * c1.g + (1 << 2) * c1.b + (1 << 3) * c2.r + (1 << 4) * c2.g + (1 << 5) * c2.b + (1 << 6) * c3.r + (1 << 7) * c3.g + (1 << 8) * c3.b;
                break;
            }
        }
        else
        {
            isOutside = true;
            // This calculates how far a mip map level should jump
            t += mod(floor(-sRayDir * rayPos), (1 << (count - 1))) * aRayDir; // This uses the number of mip maps where there are no voxels, to determine how far to jump
        }

        // Find which jump amount to use next
        float minT = min(min(t.x, t.y), t.z);
        hit.normal = -sRayDir * ivec3(minT == t.x, minT == t.y, minT == t.z); // Set the normal

        rayPos += rayDir * (minT)-hit.normal * 0.001; // 0.001 is for numerical stability (yes it causes a small aliasing artifact)
    }

    hit.hitLocation = rayPos;
    hit.dist = length(rayPos - rayStart);

    return hit;
}

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
    vec3 pos = camPos;
    // pos = 0.5 * vec3(cos(time * 0.02), 0, sin(time * 0.02)) * (sin(1.61803398875 * time * 0.02) * 0.5 + 0.5);
    vec3 forward = camDir;
    forward /= length(forward);
    vec3 right = cross(forward, vec3(0, 0, 1));
    right /= length(right);
    vec3 up = cross(right, forward);
    up /= length(up);

    float fov = (3.1415926589 / 2.f) * 1.36;
    float z = tan(fov * 0.5);

    vec2 uv = gl_FragCoord.xy / resolution - 0.5;
    uv.y *= resolution.y / resolution.x;

	
	int voxelsPerMeter = 16;//voxels per meter
	vec3 rayPos = (pos + 0.5) * voxelsPerMeter;
	vec3 rayDir = forward + 2.f * z * (uv.x * right + uv.y * up);
	rayDir /= length(rayDir);

    rayPos += rayDir * 1;

    //rayPos += (forward + 2.f * z * (uv.x * right + uv.y * up)) * 1;//This would set a near clipping plane at 1 units away
	

    RayHit hit = findIntersection2(rayPos, rayDir);
    //From here we render out the information we need to the frame buffers we need.
    //At the moment we just render to the window in a manner that provides debug information.

    if (!isWorkload)
    {
        //Show the value of the material information in the three textures by showing as different color channels.
        vec3 color = vec3((hit.material & 7) / 7.0, ((hit.material & 56) >> 3) / 7.0, ((hit.material & 448) >> 6) / 7.0);
        fragColor = vec4(color * int(hit.wasHit), 1);
    }
    else
    {

        uint r = (hit.material & 1) + ((hit.material & 8) >> 2) + ((hit.material & 64) >> 4);
        uint g = ((hit.material & (1 << 1)) >> 1) + ((hit.material & (8 << 1)) >> 3) + ((hit.material & (64 << 1)) >> 5);
        uint b = ((hit.material & (1 << 2)) >> 2) + ((hit.material & (8 << 2)) >> 4) + ((hit.material & (64 << 2)) >> 6);

        vec3 color = vec3(r / 7.0, g / 7.0, b / 7.0);
        fragColor = vec4(color * int(hit.wasHit), 1);

        /*
        //This is the workload rendering code
        fragColor = vec4(vec3(hit.iterations / 100.f), 1);

        if (hit.iterations > 100)
        {
            int temp = min(200, hit.iterations);
            fragColor = vec4(hueToRGB(0.5 - (hit.iterations - 100) / 200.f), 1);
        }
        */
        
    }

    /*
        if((hit.mipMapsUsed & (1 << 6)) > 0){
                fragColor = vec4(0, 100.f / hit.dist, 0, 1);
        }
    */

    // 3 pixel radius cursor
    if (length(gl_FragCoord.xy - resolution * 0.5) < 3)
    {
        fragColor = vec4(1);
    }
}
