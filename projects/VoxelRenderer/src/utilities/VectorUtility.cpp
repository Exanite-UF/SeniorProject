#include <src/utilities/VectorUtility.h>

#include <algorithm>
#include <sstream>
#include <src/utilities/Log.h>

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