#pragma once

#include <glm/vec2.hpp>
#include <vector>

class GeometryUtility
{
public:
    static std::vector<glm::vec2> getConvexHull(std::vector<glm::vec2> vertices);
};
