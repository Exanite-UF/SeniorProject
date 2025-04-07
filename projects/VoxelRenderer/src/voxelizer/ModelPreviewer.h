#pragma once

#include <src/utilities/ImGui.h>
#include <src/utilities/OpenGl.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <src/voxelizer/Model.h>
#include <src/voxelizer/ModelVoxelizer.h>
#include <src/windowing/Window.h>

#include <atomic>
#include <thread>

class ModelPreviewer
{
private:
    std::shared_ptr<Window> triangleWindow {};
    std::shared_ptr<Window> voxelWindow {};

    std::atomic<bool> triangleThreadRunning { false };
    std::atomic<bool> voxelThreadRunning { false };
    std::thread triangleThread {};
    std::thread voxelThread {};

    // Camera Attributes
    glm::vec3 cameraPosition = glm::vec3(-5.0f, 0.0f, 5.0f);
    glm::vec3 cameraForwardDirection = glm::vec3(0.5f, 0.0f, -0.5f);
    glm::vec3 cameraUpDirection = glm::vec3(0.0f, 1.0f, 0.0f);

    std::shared_ptr<Model> loadedModel {};

    std::shared_ptr<ShaderProgram> triangleShader {};
    std::shared_ptr<ShaderProgram> voxelShader {};

    std::shared_ptr<ModelVoxelizer> modelVoxelizer {};

public:
    ~ModelPreviewer();

    // Set instead of load since the voxelizer should be the one to load
    void setModel(const std::shared_ptr<Model>& model);

    void createTriangleWindow(const std::shared_ptr<Window>& mainWindow, const std::shared_ptr<ModelVoxelizer>& modelVoxelizer, std::string modelPath);
    void createVoxelWindow(const std::shared_ptr<Window>& mainWindow, const std::shared_ptr<ModelVoxelizer>& modelVoxelizer);

    void renderTriangleWindow();
    void renderVoxelWindow();

    void closeWindowTriangle();
    void closeWindowVoxel();
};
