#include "GeometryUtility.h"

#include <algorithm>
#include <stack>

#include "glm/gtx/compatibility.hpp"

std::vector<glm::vec2> GeometryUtility::getConvexHull(std::vector<glm::vec2> vertices)
{
    std::vector<glm::vec2> results {};

    // Find left most vertex with the lowest y-coordinate (tie-breaker)
    int index = 0;
    auto leftMostVertex = vertices.at(0);

    for (int i = 1; i < vertices.size(); ++i)
    {
        auto& candidate = vertices[i];
        if (candidate.x < leftMostVertex.x || (candidate.x == leftMostVertex.x && candidate.y < leftMostVertex.y))
        {
            index = i;
            leftMostVertex = candidate;
        }
    }

    // Order of travel is a->b->c
    // Returned values:
    // >0 when the turn is counter-clockwise
    // =0 when there is no turn
    // <0 when the turn is clockwise
    auto getOrientation = [](const glm::vec2 a, const glm::vec2 b, const glm::vec2 c)
    {
        return (b.y - a.y) * (c.x - b.x) - (b.x - a.x) * (c.y - b.y);
    };

    auto compareOrientation = [&leftMostVertex, &getOrientation](const glm::vec2 a, const glm::vec2 b)
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
