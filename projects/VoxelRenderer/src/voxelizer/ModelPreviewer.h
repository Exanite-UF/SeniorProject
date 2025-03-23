#pragma once

#include <src/utilities/ImGui.h>
#include <src/utilities/OpenGl.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <src/voxelizer/Shader.h>
#include <src/voxelizer/Model.h>


class ModelPreviewer
{
private:
    glm::ivec2 windowSize = {320, 240};
    GLFWwindow* triangleWindow = nullptr;
    GLFWwindow* voxelWindow = nullptr;

    Model* loadedModel = nullptr; // might want to replace with shared ptr


public:

    ~ModelPreviewer();

    // Set instead of load since the voxelizer should be the one to load
    void setModel(Model* model_);

    void CreateWindowTriangle();
    void CreateWindowVoxel();

    void RenderWindowTriangle();
    void RenderWindowVoxel();
};