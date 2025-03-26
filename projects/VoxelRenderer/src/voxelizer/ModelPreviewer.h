#pragma once

#include <src/utilities/ImGui.h>
#include <src/utilities/OpenGl.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <src/windowing/Window.h>
#include <src/voxelizer/Shader.h>
#include <src/voxelizer/Model.h>
#include <src/voxelizer/ModelVoxelizer.h>

#include <thread> 
#include <atomic>


class ModelPreviewer
{
private:
    glm::ivec2 windowSize = {320, 240};
    GLFWwindow* triangleWindow = nullptr;
    GLFWwindow* voxelWindow = nullptr;

    std::atomic<bool> triangleThreadRunning{false};
    std::atomic<bool> voxelThreadRunning{false};
    std::thread triangleThread;
    std::thread voxelThread;

    // Camera Attributes
    glm::vec3 Position = glm::vec3(0.0f, 0.0f, 3.0f);
    glm::vec3 Front = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 Up = glm::vec3(0.0f, 1.0f, 0.0f);

    Model* loadedModel = nullptr; // might want to replace with shared ptr
    std::string vertShaderPath = "R:/Code/SeniorProject/projects/VoxelRenderer/content/Triangulation/Phong.vertex.glsl";
    std::string fragShaderPath = "R:/Code/SeniorProject/projects/VoxelRenderer/content/Triangulation/Phong.fragment.glsl";
    Shader* shader = nullptr;


public:

    ~ModelPreviewer();

    // Set instead of load since the voxelizer should be the one to load
    void setModel(Model* model_);

    void CreateWindowTriangle(ModelVoxelizer* modelVox, std::string modelPath);
    void CreateWindowVoxel();

    void RenderWindowTriangle();
    void RenderWindowVoxel();

    void CloseWindowTriangle();
    void CloseWindowVoxel();
};