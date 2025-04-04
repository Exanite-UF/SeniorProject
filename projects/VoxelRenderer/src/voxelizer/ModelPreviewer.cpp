#include <chrono>
#include <src/Content.h>
#include <src/graphics/ShaderManager.h>
#include <src/voxelizer/ModelPreviewer.h>
#include <thread>

ModelPreviewer::~ModelPreviewer()
{
    closeWindowTriangle();
    closeWindowVoxel();
}

void ModelPreviewer::setModel(const std::shared_ptr<Model>& model)
{
    loadedModel = model;

    triangleShader = ShaderManager::getInstance().getGraphicsProgram(Content::Triangulation::vertShaderPathTriangle, Content::Triangulation::fragShaderPathTriangle);
    voxelShader = ShaderManager::getInstance().getGraphicsProgram(Content::Triangulation::vertShaderPathVoxel, Content::Triangulation::fragShaderPathVoxel);
}

void ModelPreviewer::createWindowTriangle(const std::shared_ptr<Window>& mainWindow, const std::shared_ptr<ModelVoxelizer>& modelVox, std::string modelPath)
{
    if (triangleThreadRunning)
    {
        return;
    }

    this->modelVox = modelVox;

    // Create Triangle Window in the Main Thread
    triangleWindow = std::make_shared<Window>("Voxelizer triangle window", mainWindow.get());
    mainWindow->makeContextCurrent();

    triangleThreadRunning = true;
    triangleThread = std::thread([this, modelPath, modelVox]()
        {
            triangleWindow->makeContextCurrent();

            modelVox->loadModel(const_cast<char*>(modelPath.c_str()));
            setModel(modelVox->getModel());

            // Render Loop
            while (triangleThreadRunning && !glfwWindowShouldClose(triangleWindow->getGlfwWindowHandle()))
            {
                if (glfwWindowShouldClose(triangleWindow->getGlfwWindowHandle()))
                {
                    triangleThreadRunning = false;
                    break;
                }

                triangleWindow->makeContextCurrent();

                triangleWindow->update();

                // Initial render
                renderWindowTriangle();

                static auto last_frame = std::chrono::steady_clock::now();
                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_frame);
                if (elapsed.count() < 33)
                { // ~30 FPS
                    std::this_thread::sleep_for(std::chrono::milliseconds(33 - elapsed.count()));
                }
                last_frame = now;

                triangleWindow->present(); // Swap buffers to display the rendered content
            }

            // Cleanup in the rendering thread
            triangleThreadRunning = false;
        });
}

void ModelPreviewer::createWindowVoxel(const std::shared_ptr<Window>& mainWindow, const std::shared_ptr<ModelVoxelizer>& modelVox)
{
    if (voxelThreadRunning)
    {
        return;
    }

    this->modelVox = modelVox;

    // Create Voxel Window in the Main Thread
    voxelWindow = std::make_shared<Window>("Voxelizer voxel window", mainWindow.get());
    mainWindow->makeContextCurrent();

    voxelThreadRunning = true;
    voxelThread = std::thread([this, modelVox]()
        {
            voxelWindow->makeContextCurrent();

            printf("STARTING RENDER LOOP\n");

            // generate voxels
            modelVox->voxelizeModel();

            // Render Loop
            while (voxelThreadRunning && !glfwWindowShouldClose(voxelWindow->getGlfwWindowHandle()))
            {
                if (glfwWindowShouldClose(voxelWindow->getGlfwWindowHandle()))
                {
                    voxelThreadRunning = false;
                    break;
                }

                voxelWindow->update();

                // Initial render
                renderWindowVoxel();

                static auto last_frame = std::chrono::steady_clock::now();
                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_frame);
                if (elapsed.count() < 33)
                { // ~30 FPS
                    std::this_thread::sleep_for(std::chrono::milliseconds(33 - elapsed.count()));
                }
                last_frame = now;

                voxelWindow->present(); // Swap buffers to display the rendered content
            }

            // Cleanup in the rendering thread
            voxelThreadRunning = false;
        });
}

void ModelPreviewer::renderWindowTriangle()
{
    if (triangleWindow && loadedModel && !glfwWindowShouldClose(triangleWindow->getGlfwWindowHandle()))
    {
        triangleWindow->makeContextCurrent();
        glClearColor(0.2f, 0.2f, 0.2f, 0.2f);
        glEnable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear depth?

        loadedModel->draw(triangleShader, cameraPosition, cameraForwardDirection, cameraUpDirection, windowSize.x, windowSize.y);
    }
}

void ModelPreviewer::renderWindowVoxel()
{
    if (voxelWindow && modelVox && modelVox->isVoxelized && !glfwWindowShouldClose(voxelWindow->getGlfwWindowHandle()))
    {
        triangleWindow->makeContextCurrent();

        glClearColor(0.2f, 0.2f, 0.2f, 0.2f);
        glEnable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear depth?

        modelVox->drawVoxels(voxelShader, cameraPosition, cameraForwardDirection, cameraUpDirection, windowSize.x, windowSize.y);
    }
}

void ModelPreviewer::closeWindowTriangle()
{
    if (!triangleThreadRunning)
        return;

    modelVox = nullptr;
    // Signal thead to stop
    triangleThreadRunning = false;

    // Ensure window gets closed
    if (triangleWindow)
    {
        glfwSetWindowShouldClose(triangleWindow->getGlfwWindowHandle(), true);
    }

    // Wait for thread to finish
    if (triangleThread.joinable())
    {
        triangleThread.join();
    }
}

void ModelPreviewer::closeWindowVoxel()
{
    if (!voxelThreadRunning)
        return;

    modelVox = nullptr;
    // Signal thead to stop
    voxelThreadRunning = false;

    // Ensure window gets closed
    if (voxelWindow)
    {
        glfwSetWindowShouldClose(voxelWindow->getGlfwWindowHandle(), true);
    }

    // Wait for thread to finish
    if (voxelThread.joinable())
    {
        voxelThread.join();
    }
}
