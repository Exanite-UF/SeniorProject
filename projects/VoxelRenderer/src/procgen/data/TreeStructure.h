#pragma once

#include <memory>
#include <src/world/VoxelChunkData.h>

class TreeStructure
{
public:
    glm::ivec3 originVoxel;

private:
    std::shared_ptr<Material> logMaterial;
    std::shared_ptr<Material> leafMaterial;
    int treeHeightVoxels;
    int treeWidthVoxels;
    int leafWidthX;
    int leafWidthY;
    int leafExtentBelowZ;
    int leafExtentAboveZ;
    float leafProbabilityToFill;

    glm::ivec2 maxDistanceFromOrigin;

    // TODO: Fix vec3 to ivec3
    void generateRectangle(VoxelChunkData& chunkData, glm::vec3 originVoxel, std::shared_ptr<Material>& material, int widthX, int widthY, int height);
    void generateAbsPyramid(VoxelChunkData& chunkData, glm::vec3 originVoxel, std::shared_ptr<Material>& material, int widthX, int widthY, int extentAboveZ, int extentBelowZ, float probabilityToFill);

public:
    TreeStructure(
        glm::ivec3 originVoxel,
        std::shared_ptr<Material> logMaterial,
        std::shared_ptr<Material> leafMaterial,
        int treeHeightVoxels,
        int treeWidthVoxels,
        int leafWidthX,
        int leafWidthY,
        int leafExtentBelowZ,
        int leafExtentAboveZ,
        float leafProbabilityToFill);

    void generate(VoxelChunkData& chunkData);
    [[nodiscard]] const glm::ivec3& getOriginVoxel();
    glm::ivec2 getMaxDistanceFromOrigin();
};
