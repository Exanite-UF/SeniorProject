#include "VoxelChunkComponent.h"

#include <src/Constants.h>
#include <tracy/Tracy.hpp>
#include <algorithm>
#include <glm/glm.hpp>

#include <src/gameobjects/TransformComponent.h>
#include "VoxelChunkUtility.h"

struct RayHit
{
    bool wasHit;
    glm::vec3 hitLocation;
    glm::vec3 normal;
    float dist;
    std::int32_t iterations;
    glm::vec3 voxelHitLocation;
    bool isNearest;
};

float rayboxintersect(glm::vec3 raypos, glm::vec3 raydir, glm::vec3 boxmin, glm::vec3 boxmax)
{
    if (glm::all(glm::greaterThan(raypos, boxmin)) && glm::all(glm::lessThan(raypos, boxmax)))
    { // ray is inside the box
        return 0;
    }

    float t1 = (boxmin.x - raypos.x) / raydir.x;
    float t2 = (boxmax.x - raypos.x) / raydir.x;
    float t3 = (boxmin.y - raypos.y) / raydir.y;
    float t4 = (boxmax.y - raypos.y) / raydir.y;
    float t5 = (boxmin.z - raypos.z) / raydir.z;
    float t6 = (boxmax.z - raypos.z) / raydir.z;

    float tmin = std::max(std::max(std::min(t1, t2), std::min(t3, t4)), std::min(t5, t6));
    float tmax = std::min(std::min(std::max(t1, t2), std::max(t3, t4)), std::max(t5, t6));

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


VoxelChunkComponent::VoxelChunkComponent()
    : VoxelChunkComponent(false)
{
}

VoxelChunkComponent::VoxelChunkComponent(const bool shouldGeneratePlaceholderData)
{
    ZoneScoped;

    if (shouldGeneratePlaceholderData)
    {
        chunk = std::make_unique<VoxelChunk>(Constants::VoxelChunkComponent::chunkSize, shouldGeneratePlaceholderData);
        chunkData.copyFrom(*chunk.value());
    }
}

const std::unique_ptr<VoxelChunk>& VoxelChunkComponent::getChunk()
{
    return chunk.value();
}

const VoxelChunkData& VoxelChunkComponent::getChunkData()
{
    return chunkData;
}

std::shared_mutex& VoxelChunkComponent::getMutex()
{
    return mutex;
}

VoxelChunkData& VoxelChunkComponent::getRawChunkData()
{
    return chunkData;
}

VoxelChunkComponent::RendererData& VoxelChunkComponent::getRendererData()
{
    return rendererData;
}

VoxelChunkComponent::ChunkManagerData& VoxelChunkComponent::getChunkManagerData()
{
    return chunkManagerData;
}

bool VoxelChunkComponent::getExistsOnGpu() const
{
    return chunk.has_value();
}

std::pair<float, glm::vec3> VoxelChunkComponent::raycast(glm::vec3 start, glm::vec3 direction, float currentDepth)
{
    RayHit hit;
    hit.iterations = 0;
    hit.wasHit = false;
    hit.isNearest = true;

    if (currentDepth <= 0)
    {
        currentDepth = std::numeric_limits<float>::infinity();
    }

    glm::ivec3 cellCount = chunkData.getSize() / 2;

    glm::vec3 voxelWorldScale = getTransform()->getLossyGlobalScale();

    std::uint32_t occupancyMapLayerCount = VoxelChunkUtility::getOccupancyMapIndices(chunkData.getSize()).size() - 2;

    if(!chunkData.getHasMipmaps()){
        occupancyMapLayerCount = 0;
    }

    direction /= glm::length(direction);

    glm::vec3 aRayDir = 1.f / glm::abs(direction); // This is a constant that is used several times
    glm::ivec3 sRayDir = glm::ivec3(1.5f * direction / abs(direction)); // This is the sign of the ray direction (1.5 is for numerical stability)
    glm::vec3 iRayDir = 1.f / direction;

    glm::ivec3 size = 2 * cellCount; // This is the size of the voxel volume

    glm::vec3 rayPos = start;
    glm::vec3 rayStart = rayPos;

    // Put the ray at the surface of the cube
    float distToCube = rayboxintersect(rayStart, direction, glm::vec3(0), glm::vec3(size));
    rayPos += direction * std::max(0.f, distToCube - 0.1f); // The -0.001 is for numerical stability when entering the volume (This is the aformentioned correction)

    // If the ray never entered the cube, then quit
    if (distToCube < 0)
    {
        hit.isNearest = false;
        return {-1, glm::vec3(0)};
    }

    float depth = length(direction * voxelWorldScale * distToCube); // Find how far the ray has traveled from the start

    // If the start of the voxel volume is behind the currently closest thing, then there is not reason to continue
    if (depth > currentDepth)
    {
        hit.isNearest = false;
        return {-1, glm::vec3(0)};
    }

    bool isOutside = true; // Used to make the image appear to be backface culled (It actually drastically decreases performance if rendered from inside the voxels)
    bool hasEntered = false;
    // hit.wasHit = true;
    // hit.hitLocation = rayPos;
    // hit.dist = length(rayStart - hit.hitLocation);
    // return hit;

    //This number is iterations is guarenteed to go through the entire chunk
    for (std::int32_t i = 0; i < cellCount.x * 3; i++)
    {
        hit.iterations = i;
        glm::ivec3 p = glm::ivec3(glm::floor(rayPos)); // voxel coordinate

        glm::vec3 t = glm::ceil(rayPos * glm::vec3(sRayDir)) * aRayDir - rayPos * iRayDir;
        t += glm::vec3(glm::lessThanEqual(t, glm::vec3(0))) * aRayDir; // Numerical stability correction

        // Stop iterating if you leave the cube that all the voxels are in (1 unit of padding is provided to help with numerical stability)
        bool isOutsideVolume = (glm::any(glm::greaterThan(p, glm::ivec3(size - 1))) || glm::any(glm::lessThan(p, glm::ivec3(0))));
        if (!isOutsideVolume)
        {
            hasEntered = true;
        }

        if ((i > 0) && isOutsideVolume && hasEntered)
        {
            // No voxel was hit
            break;
        }

        std::int32_t count = 0;
        // The <= is correct
        // iterate by 1 until it enters the voxel volume
        if (isOutsideVolume)
        {
            count = 1;
        }
        else
        {
            for (std::int32_t i = 0; i <= occupancyMapLayerCount; i++)
            {
                glm::ivec3 p2 = (p >> (2 * i)) & 1;
                std::uint32_t k = ((1 << p2.x) << (p2.y << 1)) << (p2.z << 2); // This creates the mask that will extract the single bit that we want
                std::uint32_t l = getOccupancyByte((p >> (1 + 2 * i)), i);
                count += int((l & k) == 0) + int(l == 0);
            }
        }

        if (count <= 0)
        {

            // This means that there was a hit
            if (i > 0 && isOutside && !isOutsideVolume)
            { // Don't intersect with the first voxel
                hit.wasHit = true;
                break;
            }
        }
        else
        {
            isOutside = true;
            // This calculates how far a mip map level should jump
            t += glm::mod(glm::floor(-glm::vec3(sRayDir) * rayPos), glm::vec3(1 << (count - 1))) * aRayDir; // This uses the number of mip maps where there are no voxels, to determine how far to jump
        }

        // Find which jump amount to use next
        float minT = glm::min(glm::min(t.x, t.y), t.z);
        hit.normal = -sRayDir * glm::ivec3(minT == t.x, minT == t.y, minT == t.z); // Set the normal

        rayPos += direction * (minT)-hit.normal * 0.001f; // 0.001 is for numerical stability (yes it causes a small aliasing artifact)
    }

    hit.hitLocation = rayPos;
    hit.dist = glm::length(rayStart - hit.hitLocation);

    return {hit.dist, hit.hitLocation};
}

void VoxelChunkComponent::allocateGpuData(const glm::ivec3& size)
{
    ZoneScoped;

    assertIsPartOfWorld();

    if (!chunk.has_value())
    {
        chunk = std::make_unique<VoxelChunk>(size, false);
    }
    else
    {
        chunk.value()->setSize(size);
    }
}

void VoxelChunkComponent::deallocateGpuData()
{
    ZoneScoped;

    if (chunk.has_value())
    {
        chunk.reset();
    }
}

// coord is a cell coord
std::uint8_t VoxelChunkComponent::getOccupancyByte(glm::ivec3 coord, int mipMapTexture)
{
    glm::ivec3 tempRes = (chunkData.getSize() / 2) / (1 << (2 * mipMapTexture)); // get the resolution of the requested mipmap
    int index = (coord.x + tempRes.x * (coord.y + tempRes.y * coord.z)) + int(chunkData.data.occupancyMapIndices[mipMapTexture]);
    int bufferIndex = index / 4; // Divide by 4, because glsl does not support single byte data types, so a 4 byte data type is being used
    int bufferOffset = (index & 3); // Modulus 4 done using a bitmask

    return (chunkData.data.occupancyMap[bufferIndex] & (255 << (8 * bufferOffset))) >> (8 * bufferOffset);
}

void VoxelChunkComponent::onRemovingFromWorld()
{
    ZoneScoped;

    std::lock_guard lock(getMutex());

    deallocateGpuData();

    Component::onRemovingFromWorld();
}
