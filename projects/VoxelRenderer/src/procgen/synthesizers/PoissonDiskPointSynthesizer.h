#pragma once

#include <src/procgen/synthesizers/PointSynthesizer.h>
#include <vector>
#include <glm/vec3.hpp>

class PoissonDiskPointSynthesizer : public PointSynthesizer
{
private:
	int seed;

public:
	PoissonDiskPointSynthesizer(int seed);

	void generatePoints(std::vector<glm::vec3>& inPoints, uint32_t numPoints);
	void showDebugMenu() override;
};