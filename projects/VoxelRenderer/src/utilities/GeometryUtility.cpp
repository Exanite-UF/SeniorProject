#include "GeometryUtility.h"

#include <algorithm>
#include <glm/gtx/compatibility.hpp>
#include <src/utilities/Assert.h>

int GeometryUtility::getOrientation(const glm::vec2& a, const glm::vec2& b, const glm::vec2& c)
{
    return glm::sign((b.y - a.y) * (c.x - b.x) - (b.x - a.x) * (c.y - b.y));
}

std::vector<glm::vec2> GeometryUtility::getConvexHull(std::vector<glm::vec2> vertices)
{
    std::vector<glm::vec2> results {};

    // Find left most vertex with the lowest y-coordinate (tie-breaker)
    auto leftMostVertex = vertices.at(0);
    for (int i = 1; i < vertices.size(); ++i)
    {
        auto& candidate = vertices[i];
        if (candidate.x < leftMostVertex.x || (candidate.x == leftMostVertex.x && candidate.y < leftMostVertex.y))
        {
            leftMostVertex = candidate;
        }
    }

    auto compareOrientation = [&leftMostVertex](const glm::vec2 a, const glm::vec2 b)
    {
        auto orientation = getOrientation(leftMostVertex, a, b);
        if (orientation == 0)
        {
            return glm::distance(leftMostVertex, a) < glm::distance(leftMostVertex, b);
        }

        return orientation > 0;
    };

    // Sort vertices by polar angle
    // The result should have vertices ordered in ascending polar angle (counter-clockwise)
    std::sort(vertices.begin(), vertices.end(), compareOrientation);

    for (auto vertex : vertices)
    {
        while (results.size() > 1)
        {
            auto previousPoint1 = results[results.size() - 1];
            auto previousPoint2 = results[results.size() - 2];

            // Check if we need to turn clockwise
            auto orientation = getOrientation(previousPoint2, previousPoint1, vertex);
            if (orientation > 0)
            {
                break;
            }

            results.pop_back();
        }

        results.push_back(vertex);
    }

    return results;
}

bool GeometryUtility::isPointInsideConvexPolygon(const glm::vec2& point, const std::vector<glm::vec2>& vertices)
{
    Assert::isTrue(vertices.size() >= 3, "Not a polygon");

    auto expectedOrientation = getOrientation(point, vertices[0], vertices[1]);

    // Check if the point is on the same side of each of the polygon edges
    for (int i = 1; i < vertices.size(); ++i)
    {
        auto& vertex1 = vertices[i];
        auto& vertex2 = vertices[(i + 1) % vertices.size()];
        auto orientation = getOrientation(point, vertex1, vertex2);

        if (orientation != expectedOrientation)
        {
            return false;
        }
    }

    return true;
}
