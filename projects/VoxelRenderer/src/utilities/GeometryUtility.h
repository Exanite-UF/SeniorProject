#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <vector>

class GeometryUtility
{
private:
    // Order of travel is a->b->c
    // Returned values:
    // 1 when the turn is counter-clockwise
    // 0 when there is no turn
    // -1 when the turn is clockwise
    static int getOrientation(const glm::vec2& a, const glm::vec2& b, const glm::vec2& c);

public:
    static std::vector<glm::vec2> getConvexHull(std::vector<glm::vec2> vertices);

    static bool isPointInsideConvexPolygon(const glm::vec2& point, const std::vector<glm::vec2>& vertices);

    static void lexicographicSort(std::vector<glm::vec3>& points);
};
