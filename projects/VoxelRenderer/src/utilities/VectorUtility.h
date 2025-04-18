#pragma once

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <vector>
#include <string>

class VectorUtility
{
public:
    static void printVec2(std::string name, glm::vec2 position);
    static void printVec3(std::string name, glm::vec3 position);
};
