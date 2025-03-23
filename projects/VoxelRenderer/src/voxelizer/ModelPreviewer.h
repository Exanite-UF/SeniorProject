#pragma once

#include <src/utilities/ImGui.h>
#include <src/utilities/OpenGl.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>



class ModelPreviewer
{
private:
    glm::ivec2 windowSize = {320, 240};
    GLFWwindow* triangleWindow = nullptr;
    GLFWwindow* voxelWindow = nullptr;

public:
    void CreateWindowTriangle();
    void CreateWindowVoxel();

    void RenderWindowTriangle();
    void RenderWindowVoxel();
};