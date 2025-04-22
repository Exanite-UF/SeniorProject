#pragma once

#include <src/procgen/generators/WorldGenerator.h>
#include <src/procgen/synthesizers/PointSynthesizer.h>

class PointSynthesizerWorldGenerator : public WorldGenerator
{
protected:
    std::shared_ptr<PointSynthesizer> pointSynthesizer;

    int seed = 3;
    int pillarHeight = 50;

    void generateData(VoxelChunkData& data) override;

public:
    explicit PointSynthesizerWorldGenerator(const std::shared_ptr<PointSynthesizer>& pointSynthesizer);

    void showDebugMenu() override;
};
