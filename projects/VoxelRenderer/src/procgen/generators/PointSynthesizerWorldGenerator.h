#pragma once

#include <src/procgen/generators/WorldGenerator.h>
#include <src/procgen/synthesizers/PointSynthesizer.h>
#include <src/utilities/VectorUtility.h>

class PointSynthesizerWorldGenerator : public WorldGenerator
{
protected:
	std::shared_ptr<PointSynthesizer> pointSynthesizer;

	int seed = 3;
	int height = 50;

	void generateData(VoxelChunkData& data) override;	

public:
	PointSynthesizerWorldGenerator(const std::shared_ptr<PointSynthesizer>& pointSynthesizer);
    void showDebugMenu() override;
};