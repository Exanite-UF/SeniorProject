#include <src/procgen/generators/PointSynthesizerWorldGenerator.h>

#include <algorithm>
#include <src/procgen/WorldUtility.h>
#include <src/procgen/synthesizers/PoissonDiskPointSynthesizer.h>
#include <src/utilities/ImGui.h>
#include <src/utilities/Log.h>
#include <src/world/MaterialManager.h>
#include <src/world/VoxelChunkUtility.h>

PointSynthesizerWorldGenerator::PointSynthesizerWorldGenerator(const std::shared_ptr<PointSynthesizer>& pointSynthesizer)
{
    this->pointSynthesizer = pointSynthesizer;
}

void PointSynthesizerWorldGenerator::generateData(VoxelChunkData& data)
{
    auto& materialManager = MaterialManager::getInstance();

    std::shared_ptr<Material> whiteMaterial;
    WorldUtility::tryGetMaterial("greyscale_255", materialManager, whiteMaterial);

    std::shared_ptr<Material> blackMaterial;
    WorldUtility::tryGetMaterial("greyscale_0", materialManager, blackMaterial);

    std::vector<glm::vec3> points;
    pointSynthesizer->setSeed(seed);
    pointSynthesizer->generatePoints(points, 20);
    pointSynthesizer->rescalePointsToChunkSize(points, data);

    int pointIndex = 0;
    glm::vec3 nextVoxel = points[0];

    for (int x = 0; x < data.getSize().x; ++x)
    {
        for (int y = 0; y < data.getSize().y; ++y)
        {
            glm::vec3 voxel = { x, y, 1 };

            data.setVoxelOccupancy(voxel, true);
            data.setVoxelMaterial(voxel, whiteMaterial);
        }
    }

    for (int i = 0; i < points.size(); i++)
    {
        glm::ivec3 point = glm::ivec3(points[i]);

        for (int z = 0; z <= pillarHeight; ++z)
        {
            glm::ivec3 voxel = { point.x, point.y, z };

            if (VoxelChunkUtility::isValidPosition(voxel, data.getSize()))
            {
                data.setVoxelOccupancy(voxel, true);
                data.setVoxelMaterial(voxel, blackMaterial);
            }
        }
    }
}

void PointSynthesizerWorldGenerator::showDebugMenu()
{
    ImGui::PushID("PointSynthesizerWorldGenerator");
    {
        if (ImGui::CollapsingHeader("Poisson Disk Pillar Visual Generator (F9)"))
        {
            if (ImGui::BeginMenu("Value"))
            {
                ImGui::SliderInt("Seed", &seed, 1, 100);
                ImGui::SliderInt("Pillar Height", &pillarHeight, 1, 100);
                ImGui::EndMenu();
            }
        }
    }
    ImGui::PopID();
}
