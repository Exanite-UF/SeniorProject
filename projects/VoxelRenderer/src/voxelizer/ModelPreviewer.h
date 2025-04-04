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

    std::shared_ptr<Model> loadedModel {};

    std::shared_ptr<Shader> triangleShader {};
    std::shared_ptr<Shader> voxelShader {};

    std::shared_ptr<ModelVoxelizer> modelVox {};

public:
    ~ModelPreviewer();

    // Set instead of load since the voxelizer should be the one to load
    void setModel(const std::shared_ptr<Model>& model);

    void CreateWindowTriangle(const std::shared_ptr<Window>& mainWindow, const std::shared_ptr<ModelVoxelizer>& modelVox, std::string modelPath);
    void CreateWindowVoxel(const std::shared_ptr<Window>& mainWindow, const std::shared_ptr<ModelVoxelizer>& modelVox);

    void RenderWindowTriangle();
    void RenderWindowVoxel();

    void CloseWindowTriangle();
    void CloseWindowVoxel();
};
