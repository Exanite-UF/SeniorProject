#include <src/utilities/VectorUtility.h>

#include <algorithm>
#include <sstream>
#include <src/utilities/Log.h>

// O(nlogn) sort
void VectorUtility::lexicographicSort(std::vector<glm::vec3>& points)
{
    std::sort(points.begin(), points.end(), [](const glm::vec3& a, const glm::vec3& b)
        {
            return a.x < b.x || (a.x == b.x && a.y < b.y) || (a.x == b.x && a.y == b.y && a.z < b.z);
        });
}

void VectorUtility::printVec2(std::string name, glm::vec2 position)
{
    std::stringstream ss;
    ss << position.x << " " << position.y << std::endl;
    Log::debug(ss.str());
}

void VectorUtility::printVec3(std::string name, glm::vec3 position)
{
    std::stringstream ss;
    ss << position.x << " " << position.y << " " << position.z << std::endl;
    Log::debug(ss.str());
}