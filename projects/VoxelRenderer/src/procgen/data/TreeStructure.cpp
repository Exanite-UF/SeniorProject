#include <src/procgen/data/TreeStructure.h>

#include <algorithm>
#include <src/world/VoxelChunkData.h>
#include <src/world/VoxelChunkUtility.h>
#include <tracy/Tracy.hpp>

TreeStructure::TreeStructure(
    glm::ivec2 originVoxel,
    std::shared_ptr<Material> logMaterial,
    std::shared_ptr<Material> leafMaterial,
    int treeHeightVoxels,
    int treeWidthVoxels,
    int leafWidthX,
    int leafWidthY,
    int leafExtentBelowZ,
    int leafExtentAboveZ,
    float leafProbabilityToFill)
    : logMaterial(logMaterial)
    , leafMaterial(leafMaterial)
{
    this->originVoxel = originVoxel;
    this->treeHeightVoxels = treeHeightVoxels;
    this->treeWidthVoxels = treeWidthVoxels;
    this->leafWidthX = leafWidthX;
    this->leafWidthY = leafWidthY;
    this->leafExtentBelowZ = leafExtentBelowZ;
    this->leafExtentAboveZ = leafExtentAboveZ;
    this->leafProbabilityToFill = leafProbabilityToFill;
    this->maxDistanceFromOrigin = (int)std::ceil(std::max(treeWidthVoxels, leafWidthX) / 2.0f);
}

void TreeStructure::generate(VoxelChunkData& chunkData, int z)
{
    ZoneScoped;

    glm::ivec3 transformPosition = { originVoxel.x, originVoxel.y, z };

    // Tree Trunk
    generateRectangle(
        chunkData,
        transformPosition,
        logMaterial,
        treeWidthVoxels,
        treeWidthVoxels,
        treeHeightVoxels);

    glm::ivec3 leafOffset = { 0, 0, treeHeightVoxels + 1 };
    transformPosition += leafOffset;

    generateAbsPyramid(
        chunkData,
        transformPosition,
        leafMaterial,
        leafWidthX,
        leafWidthY,
        leafExtentAboveZ,
        leafExtentBelowZ,
        leafProbabilityToFill);
}

const glm::ivec2& TreeStructure::getOriginVoxel()
{
    return originVoxel;
}

int TreeStructure::getMaxDistanceFromOrigin()
{
    return maxDistanceFromOrigin;
}

void TreeStructure::generateRectangle(VoxelChunkData& chunkData, glm::vec3 originVoxel, std::shared_ptr<Material>& material, int widthX, int widthY, int height)
{
    int widthXOffset = widthX / 2;
    int widthYOffset = widthY / 2;

    for (int localX = 0; localX <= widthX; ++localX)
    {
        for (int localY = 0; localY <= widthY; ++localY)
        {
            for (int localZ = 0; localZ <= height; ++localZ)
            {
                glm::vec3 localVoxel = { originVoxel.x + localX - widthXOffset, originVoxel.y + localY - widthYOffset, originVoxel.z + localZ };

                if (!VoxelChunkUtility::isValidPosition(localVoxel, chunkData.getSize()))
                {
                    continue;
                }

                chunkData.setVoxelOccupancy(localVoxel, true);
                chunkData.setVoxelMaterial(localVoxel, material);
            }
        }
    }
}

void TreeStructure::generateAbsPyramid(VoxelChunkData& chunkData, glm::vec3 originVoxel, std::shared_ptr<Material>& material, int widthX, int widthY, int extentAboveZ, int extentBelowZ, float probabilityToFill)
{
    int leafWidthRadiusX = widthX / 2;
    int leafWidthRadiusY = widthY / 2;

    // Setup tree function
    int heightZ = extentAboveZ;
    float heightToWidthXRatio = ((float)heightZ) / widthX;
    float heightToWidthYRatio = ((float)heightZ) / widthY;

    for (int localX = -leafWidthRadiusX; localX <= leafWidthRadiusX; ++localX)
    {
        for (int localY = -leafWidthRadiusY; localY <= leafWidthRadiusY; ++localY)
        {
            for (int localZ = -extentBelowZ; localZ <= extentAboveZ; ++localZ)
            {
                glm::vec3 localVoxel = { originVoxel.x + localX, originVoxel.y + localY, originVoxel.z + localZ };

                // Fall through
                if (!VoxelChunkUtility::isValidPosition(localVoxel, chunkData.getSize()))
                {
                    continue;
                }

                int treeFunctionSample = heightZ - heightToWidthXRatio * abs(localX) - heightToWidthYRatio * abs(localY);
                // Simple random function. Probably better to clump and also add so it looks more organic.
                bool randomSample = ((float)rand() / RAND_MAX) >= probabilityToFill;

                if (localZ <= treeFunctionSample && randomSample)
                {
                    if (chunkData.getVoxelMaterial(localVoxel) != material)
                    {
                        chunkData.setVoxelOccupancy(localVoxel, true);
                        chunkData.setVoxelMaterial(localVoxel, material);
                    }
                }
            }
        }
    }
}
