#pragma once

#include <src/utilities/ImGui.h>
#include <src/utilities/OpenGl.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <src/voxelizer/Model.h>
#include <src/voxelizer/ModelVoxelizer.h>
#include <src/voxelizer/Shader.h>
#include <src/windowing/Window.h>

#include <atomic>
#include <thread>

class ModelPreviewer
{
private:
    glm::ivec2 windowSize = { 320, 240 };

    std::shared_ptr<Window> triangleWindow {};
    std::shared_ptr<Window> voxelWindow {};

    std::atomic<bool> triangleThreadRunning { false };
    std::atomic<bool> voxelThreadRunning { false };
    std::thread triangleThread;
    std::thread voxelThread;

    // Camera Attributes
    glm::vec3 Position = glm::vec3(0.0f, 0.0f, 5.0f);
    glm::vec3 Front = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 Up = glm::vec3(0.0f, 1.0f, 0.0f);

    Model* loadedModel = nullptr; // might want to replace with shared ptr
    std::string vertShaderPathTriangle = "content/Triangulation/Phong.vertex.glsl";
    std::string fragShaderPathTriangle = "content/Triangulation/Phong.fragment.glsl";
    std::string vertShaderPathVoxel = "content/Triangulation/Voxel.vertex.glsl";
    std::string fragShaderPathVoxel = "content/Triangulation/Voxel.fragment.glsl";

    Shader* triangleShader = nullptr;
    Shader* voxelShader = nullptr;

    ModelVoxelizer* modelVox = nullptr;

public:
    ~ModelPreviewer();

    // Set instead of load since the voxelizer should be the one to load
    void setModel(Model* model_);

    void CreateWindowTriangle(std::shared_ptr<Window> mainWindow, ModelVoxelizer* modelVox, std::string modelPath);
    void CreateWindowVoxel(std::shared_ptr<Window> mainWindow, ModelVoxelizer* modelVox);

    void RenderWindowTriangle();
    void RenderWindowVoxel();

    void CloseWindowTriangle();
    void CloseWindowVoxel();
};
