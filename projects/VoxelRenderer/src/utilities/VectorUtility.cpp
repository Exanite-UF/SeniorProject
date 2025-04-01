#include <src/utilities/VectorUtility.h>

#include <algorithm>

// O(nlogn) sort
void VectorUtility::lexicographicSort(std::vector<glm::vec3>& points)
{
    std::sort(points.begin(), points.end(), [](const glm::vec3& a, const glm::vec3& b)
        {
            return a.x < b.x || (a.x == b.x && a.y < b.y) || (a.x == b.x && a.y == b.y && a.z < b.z);
        });
}
