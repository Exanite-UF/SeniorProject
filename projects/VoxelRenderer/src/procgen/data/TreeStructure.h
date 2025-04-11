#pragma once

#include <memory>
#include <src/world/VoxelChunkData.h>

class TreeStructure
{
private:
    glm::ivec2 originVoxel;
    std::shared_ptr<Material> logMaterial;
    std::shared_ptr<Material> leafMaterial;
    int treeHeightVoxels;
    int treeWidthVoxels;
    int leafWidthX;
    int leafWidthY;
    int leafExtentBelowZ;
    int leafExtentAboveZ;
    float leafProbabilityToFill;

    int maxDistanceFromOrigin;

    //TODO: Fix vec3 to ivec3
    void generateRectangle(VoxelChunkData& chunkData, glm::vec3 originVoxel, std::shared_ptr<Material>& material, int widthX, int widthY, int height);
    void generateAbsPyramid(VoxelChunkData& chunkData, glm::vec3 originVoxel, std::shared_ptr<Material>& material, int widthX, int widthY, int extentAboveZ, int extentBelowZ, float probabilityToFill);

public:
    TreeStructure(
        glm::ivec2 originVoxel,
        std::shared_ptr<Material> logMaterial,
        std::shared_ptr<Material> leafMaterial,
        int treeHeightVoxels,
        int treeWidthVoxels,
        int leafWidthX,
        int leafWidthY,
        int leafExtentBelowZ,
        int leafExtentAboveZ,
        float leafProbabilityToFill);

    void generate(VoxelChunkData& chunkData, int z);
    [[nodiscard]] const glm::ivec2& getOriginVoxel();
    int getMaxDistanceFromOrigin();
};
