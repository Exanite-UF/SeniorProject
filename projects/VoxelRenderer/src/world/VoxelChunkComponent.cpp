#include "VoxelChunkComponent.h"

#include <algorithm>
#include <glm/glm.hpp>
#include <src/Constants.h>
#include <tracy/Tracy.hpp>

#include <iostream>

#include "VoxelChunkUtility.h"
#include <src/gameobjects/TransformComponent.h>

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

glm::vec3 qtransform(glm::vec4 q, glm::vec3 v)
{
    glm::vec3 temp = glm::vec3(q.x, q.y, q.z);
    return v + 2.0f * glm::cross(temp, glm::cross(temp, v) + q.w * v);
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
    if (!mutex.try_lock_shared())
    {
        return { -1, glm::vec3(0) };
    }

    RayHit hit;
    hit.iterations = 0;
    hit.wasHit = false;
    hit.isNearest = true;

    glm::vec3 rayDir = direction;
    glm::vec3 rayPos = start;

    glm::ivec3 cellCount = chunkData.getSize() / 2;

    glm::vec3 voxelWorldSize = 2.f * glm::vec3(cellCount);

    glm::vec3 voxelWorldPosition = getTransform()->getGlobalPosition();
    glm::vec3 voxelWorldScale = getTransform()->getLossyGlobalScale();
    glm::vec4 voxelWorldRotation;
    {
        glm::quat temp = getTransform()->getGlobalRotation();
        voxelWorldRotation = glm::vec4(temp.x, temp.y, temp.z, temp.w);
    }

    // Transform the ray position to voxel space
    rayPos -= voxelWorldPosition; // Find position relative to the voxel world
    rayPos = qtransform(glm::vec4(voxelWorldRotation.x, voxelWorldRotation.y, voxelWorldRotation.z, -voxelWorldRotation.w), rayPos); // Inverse rotations lets us rotate from world space to voxel space
    rayPos /= voxelWorldScale; // Undo the scale now that we are aligned with voxel space
    rayPos += 0.5f * glm::vec3(voxelWorldSize); // This moves the origin of the voxel world to its center

    // Transform the ray direction to voxel space
    rayDir = qtransform(glm::vec4(voxelWorldRotation.x, voxelWorldRotation.y, voxelWorldRotation.z, -voxelWorldRotation.w), rayDir);
    rayDir /= voxelWorldScale;

    if (currentDepth <= 0)
    {
        currentDepth = std::numeric_limits<float>::infinity();
    }

    std::uint32_t occupancyMapLayerCount = chunkData.getRawOccupancyMapIndices().size() - 1;

    if (!chunkData.getHasMipmaps())
    {
        occupancyMapLayerCount = 0;
    }

    rayDir /= glm::length(rayDir);

    glm::vec3 aRayDir = glm::abs(1.f / rayDir); // This is a constant that is used several times
    glm::ivec3 sRayDir = glm::ivec3((1.5f * rayDir) / glm::abs(rayDir)); // This is the sign of the ray direction (1.5 is for numerical stability)
    glm::vec3 iRayDir = 1.f / rayDir;

    glm::ivec3 size = 2 * cellCount; // This is the size of the voxel volume

    if (sRayDir.x < -10)
    {
        sRayDir.x = 0;
    }
    if (sRayDir.y < -10)
    {
        sRayDir.y = 0;
    }
    if (sRayDir.y < -10)
    {
        sRayDir.y = 0;
    }

    glm::vec3 rayStart = rayPos;

    // Put the ray at the surface of the cube
    float distToCube = rayboxintersect(rayStart, rayDir, glm::vec3(0), glm::vec3(size));
    rayPos += rayDir * std::max(0.f, distToCube - 0.1f); // The -0.001 is for numerical stability when entering the volume (This is the aformentioned correction)

    // If the ray never entered the cube, then quit
    if (distToCube < 0)
    {
        hit.isNearest = false;
        mutex.unlock_shared();
        return { -1, glm::vec3(0) };
    }

    float depth = length(direction * voxelWorldScale * distToCube); // Find how far the ray has traveled from the start

    // If the start of the voxel volume is behind the currently closest thing, then there is not reason to continue
    if (depth > currentDepth)
    {
        hit.isNearest = false;
        mutex.unlock_shared();
        return { -1, glm::vec3(0) };
    }

    bool isOutside = true; // Used to make the image appear to be backface culled (It actually drastically decreases performance if rendered from inside the voxels)
    bool hasEntered = false;
    // hit.wasHit = true;
    // hit.hitLocation = rayPos;
    // hit.dist = length(rayStart - hit.hitLocation);
    // return hit;

    // This number is iterations is guarenteed to go through the entire chunk

    // std::cout << "START" << std::endl;
    // std::cout << occupancyMapLayerCount << std::endl;
    occupancyMapLayerCount = 0;
    for (std::int32_t i = 0; i < cellCount.x * 3; i++)
    {
        hit.iterations = i;
        glm::ivec3 p = glm::ivec3(glm::floor(rayPos)); // voxel coordinate

        glm::vec3 t = glm::ceil(rayPos * glm::vec3(sRayDir)) * aRayDir - rayPos * iRayDir;
        t += glm::vec3(glm::lessThanEqual(t, glm::vec3(0))) * aRayDir; // Numerical stability correction

        if (std::isnan(t.x) || std::isinf(t.x))
        {
            t.x = std::numeric_limits<float>::infinity();
        }
        if (std::isnan(t.y) || std::isinf(t.y))
        {
            t.y = std::numeric_limits<float>::infinity();
        }
        if (std::isnan(t.z) || std::isinf(t.z))
        {
            t.z = std::numeric_limits<float>::infinity();
        }

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
            // wstd::cout << p.x << " " << p.y << " " << p.z << std::endl;
            for (std::int32_t i = 0; i <= occupancyMapLayerCount; i++)
            {

                glm::ivec3 p2 = (p >> (2 * i)) & 1;
                std::uint32_t k = ((1 << p2.x) << (p2.y << 1)) << (p2.z << 2); // This creates the mask that will extract the single bit that we want
                std::uint32_t l = getOccupancyByte((p >> (1 + 2 * i)), i);

                // std::cout << l << std::endl;
                count += ((l & k) == 0) + (l == 0);
            }
        }

        // std::cout << count << std::endl;

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

            if (std::isnan(t.x) || std::isinf(t.x))
            {
                t.x = std::numeric_limits<float>::infinity();
            }
            if (std::isnan(t.y) || std::isinf(t.y))
            {
                t.y = std::numeric_limits<float>::infinity();
            }
            if (std::isnan(t.z) || std::isinf(t.z))
            {
                t.z = std::numeric_limits<float>::infinity();
            }
        }

        // Find which jump amount to use next
        float minT = glm::min(glm::min(t.x, t.y), t.z);
        hit.normal = -sRayDir * glm::ivec3(minT == t.x, minT == t.y, minT == t.z); // Set the normal

        rayPos += rayDir * (minT)-hit.normal * 0.001f; // 0.001 is for numerical stability (yes it causes a small aliasing artifact)
    }

    hit.hitLocation = rayPos;

    hit.hitLocation -= 0.5f * glm::vec3(voxelWorldSize); // This moves the origin of the voxel world to its center
    hit.hitLocation *= voxelWorldScale; // Apply the scale of the voxel world
    hit.hitLocation = qtransform(voxelWorldRotation, hit.hitLocation); // Rotate back into world space
    hit.hitLocation += voxelWorldPosition; // Apply the voxel world position

    hit.dist = glm::length(start - hit.hitLocation);

    // std::cout << "END " << hit.wasHit << " " << hit.dist << " | " << hit.hitLocation.x << " " << hit.hitLocation.y << " " << hit.hitLocation.z<< std::endl;

    if (!hit.wasHit)
    {
        mutex.unlock_shared();
        return { -1, glm::vec3(0) };
    }

    mutex.unlock_shared();
    return { hit.dist, hit.hitLocation };
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
    int index = (coord.x + tempRes.x * (coord.y + tempRes.y * coord.z)) + chunkData.getRawOccupancyMapIndices().at(mipMapTexture);
    // std::cout << mipMapTexture << " " << chunkData.getRawOccupancyMapIndices()[mipMapTexture] << " " << (int)chunkData.getRawOccupancyMap()[index] << std::endl;
    return chunkData.getRawOccupancyMap().at(index);
}

void VoxelChunkComponent::onRemovingFromWorld()
{
    ZoneScoped;

    std::lock_guard lock(getMutex());

    deallocateGpuData();

    Component::onRemovingFromWorld();
}
