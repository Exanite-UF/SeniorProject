#include <src/procgen/synthesizers/PoissonDiskPointSynthesizer.h>

#include <glm/vec3.hpp>
#include <PoissonDiskGenerator/PoissonGenerator.h>
#include <algorithm>
#include <typeinfo>
#include <imgui/imgui.h>

PoissonDiskPointSynthesizer::PoissonDiskPointSynthesizer(int seed)
{
	this->seed = seed;
}

void PoissonDiskPointSynthesizer::generatePoints(std::vector<glm::vec3>& inPoints, uint32_t numPoints)
{
    PoissonGenerator::DefaultPRNG PRNG(seed);
	
    // Generated points between 0-1
    const auto points = PoissonGenerator::generatePoissonPoints(numPoints, PRNG, false); 
	
	for(int i = 0; i < points.size(); i++)
	{
		inPoints.push_back(glm::vec3(points[i].x, points[i].y, 0));
	}
}

void PoissonDiskPointSynthesizer::showDebugMenu() 
{
	ImGui::PushID(typeid(PoissonDiskPointSynthesizer).name());
    {
        if (ImGui::CollapsingHeader("Texture-Heightmap World Generator"))
        {
            ImGui::SliderInt("Base Height", &seed, 0, 100);
        }
    }
    ImGui::PopID();
}
