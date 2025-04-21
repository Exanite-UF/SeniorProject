#include <chrono>
#include <src/Content.h>
#include <src/graphics/ShaderManager.h>
#include <src/utilities/OpenGl.h>
#include <src/voxelizer/ModelPreviewer.h>
#include <src/windowing/GlfwContext.h>
#include <src/windowing/Window.h>
#include <thread>




ModelPreviewer::~ModelPreviewer()
{
    rasterizationShader.reset();
    rayMarchingShader.reset();
    closeWindowTriangle();
    closeWindowVoxel();
}

void ModelPreviewer::setModel(const std::shared_ptr<Model>& model)
{
    loadedModel = model;

    triangleShader = ShaderManager::getInstance().getGraphicsProgram(Content::Triangulation::vertShaderPathTriangle, Content::Triangulation::fragShaderPathTriangle);
    voxelShader = ShaderManager::getInstance().getGraphicsProgram(Content::Triangulation::vertShaderPathVoxel, Content::Triangulation::fragShaderPathVoxel);

}

void ModelPreviewer::createTriangleWindow(const std::shared_ptr<Window>& mainWindow, const std::shared_ptr<ModelVoxelizer>& modelVoxelizer, std::string modelPath)
{
    if (triangleThreadRunning)
    {
        closeWindowTriangle();
    }

    this->modelVoxelizer = modelVoxelizer;

    // Create Triangle Window in the main thread
    triangleWindow = std::make_shared<Window>("Voxelizer triangle window", mainWindow.get(), false, false);
    triangleWindow->setWindowed(300, 300);

    // Restore original context
    mainWindow->makeContextCurrent();

    triangleThreadRunning = true;
    triangleThread = std::thread([this, modelPath, modelVoxelizer]()
        {
            triangleWindow->makeContextCurrent();

            modelVoxelizer->loadModel(const_cast<char*>(modelPath.c_str()));
            setModel(modelVoxelizer->getModel());

            // Render Loop
            while (triangleThreadRunning)
            {
                // Limit FPS
                static auto last_frame = std::chrono::steady_clock::now();
                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_frame);
                if (elapsed.count() < 33)
                {
                    // ~30 FPS
                    std::this_thread::sleep_for(std::chrono::milliseconds(33 - elapsed.count()));
                }
                last_frame = now;

                // Update
                triangleWindow->update();

                // Render
                renderTriangleWindow();

                // Present
                triangleWindow->present();
            }

            // Cleanup
            triangleThreadRunning = false;
            triangleWindow.reset();
        });
}

void ModelPreviewer::createVoxelWindow(const std::shared_ptr<Window>& mainWindow, const std::shared_ptr<ModelVoxelizer>& modelVoxelizer)
{
    if (voxelThreadRunning)
    {
        closeWindowVoxel();
    }

    this->modelVoxelizer = modelVoxelizer;

    // Create Voxel Window in the main thread
    voxelWindow = std::make_shared<Window>("Voxelizer voxel window", mainWindow.get(), false, false);
    voxelWindow->setWindowed(300, 300);
    if (!glewIsSupported("GL_ARB_shader_image_load_store")) {
        std::cerr << "GL_ARB_shader_image_load_store is not supported!" << std::endl;
    }
    
    // Restore original context
    mainWindow->makeContextCurrent();

    voxelThreadRunning = true;
    voxelThread = std::thread([this, modelVoxelizer]()
        {
            voxelWindow->makeContextCurrent();
            
            rasterizationShader = ShaderManager::getInstance().getGraphicsProgram(Content::Triangulation::vertShaderPathRasterization, Content::Triangulation::fragShaderPathRasterization);
            modelVoxelizer->rasterizationShader = rasterizationShader;

            rayMarchingShader = ShaderManager::getInstance().getComputeProgram(Content::Triangulation::compShaderPathRayMarch);
            modelVoxelizer->rayMarchingShader = rayMarchingShader;
        
            printf("STARTING RENDER LOOP\n");

            // generate voxels
            modelVoxelizer->voxelizeModel();

            // Render Loop
            while (voxelThreadRunning)
            {
                // Limit FPS
                static auto last_frame = std::chrono::steady_clock::now();
                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_frame);
                if (elapsed.count() < 33)
                {
                    // ~30 FPS
                    std::this_thread::sleep_for(std::chrono::milliseconds(33 - elapsed.count()));
                }
                last_frame = now;

                // Update
                voxelWindow->update();

                // Render
                renderVoxelWindow();

                // Present
                voxelWindow->present(); // Swap buffers to display the rendered content
            }

            // Cleanup
            voxelThreadRunning = false;
            voxelWindow.reset();
        });
}

void ModelPreviewer::renderTriangleWindow()
{
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.2f, 0.2f, 0.2f, 0);
    glClearDepth(1);
    glDepthFunc(GL_LESS);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (loadedModel)
    {
        loadedModel->draw(triangleShader, cameraPosition, cameraForwardDirection, cameraUpDirection, triangleWindow->size);
    }
}

void ModelPreviewer::renderVoxelWindow()
{
    glEnable(GL_DEPTH_TEST);

    glClearColor(0.2f, 0.2f, 0.2f, 0);
    glClearDepth(1);
    glDepthFunc(GL_LESS);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (modelVoxelizer && modelVoxelizer->isVoxelized)
    {
        modelVoxelizer->drawVoxels(voxelShader, cameraPosition, cameraForwardDirection, cameraUpDirection, voxelWindow->size);
    }
}

void ModelPreviewer::closeWindowTriangle()
{
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

void ModelPreviewer::clearResources()
{
    // Close Windows and Threads
    if (triangleThreadRunning)
    {
        closeWindowTriangle();
    }

    if (voxelThreadRunning)
    {
        GLFWwindow* curContext = glfwGetCurrentContext();
        voxelWindow->makeContextCurrent();
        modelVoxelizer->clearResources();
        if (curContext)
        {
            glfwMakeContextCurrent(curContext);
        }
        closeWindowVoxel();
    }

    triangleWindow.reset();
    voxelWindow.reset();
    loadedModel.reset();
    triangleShader.reset();
    voxelShader.reset();
}
