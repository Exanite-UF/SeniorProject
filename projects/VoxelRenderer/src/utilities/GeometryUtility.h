#pragma once

#include <glm/vec2.hpp>
#include <vector>

class GeometryUtility
{
private:
    // Order of travel is a->b->c
    // Returned values:
    // >0 when the turn is counter-clockwise
    // =0 when there is no turn
    // <0 when the turn is clockwise
    static float getOrientation(const glm::vec2& a, const glm::vec2& b, const glm::vec2& c);

public:
    static std::vector<glm::vec2> getConvexHull(std::vector<glm::vec2> vertices);
};
