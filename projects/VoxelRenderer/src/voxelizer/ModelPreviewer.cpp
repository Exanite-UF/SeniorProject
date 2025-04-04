#include <src/voxelizer/ModelPreviewer.h>
#include <thread>
#include <chrono>
#include <thread>


ModelPreviewer::~ModelPreviewer()
{
    if (loadedModel)
    {
        delete loadedModel;
        delete triangleShader;
        delete voxelShader;
    }
    CloseWindowTriangle();
    CloseWindowVoxel();
}

void ModelPreviewer::setModel(Model* model_)
{
    loadedModel = model_;
    triangleShader = new Shader(vertShaderPathTriangle.c_str(), fragShaderPathTriangle.c_str());
    voxelShader = new Shader(vertShaderPathVoxel.c_str(), fragShaderPathVoxel.c_str());
}


void ModelPreviewer::CreateWindowTriangle(std::shared_ptr<Window> mainWindow, ModelVoxelizer* modelVox_, std::string modelPath)
{
    if (triangleThreadRunning)
    {
        return;
    }

    modelVox = modelVox_;

    // Create Triangle Window in the Main Thread
    triangleWindow = std::make_shared<Window>(mainWindow.get());  

    if (!triangleWindow)
    {
        printf("FAILED TO CREATE MODEL TRIANGLE VIEW WINDOW!\n");
        return;
    }

    triangleThreadRunning = true;
    triangleThread = std::thread([this, modelPath]() {
        triangleWindow->makeContextCurrent();

        modelVox->loadModel(const_cast<char*>(modelPath.c_str()));
        setModel(modelVox->getModel());


        // Render Loop
        while (triangleThreadRunning && !glfwWindowShouldClose(triangleWindow->getGlfwWindowHandle())) {
            if (glfwWindowShouldClose(triangleWindow->getGlfwWindowHandle())) {
                triangleThreadRunning = false;
                break; 
            }

            triangleWindow->makeContextCurrent();

            triangleWindow->update();

            // Initial render
            RenderWindowTriangle();


            static auto last_frame = std::chrono::steady_clock::now();
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_frame);
            if (elapsed.count() < 33) {  // ~30 FPS
                std::this_thread::sleep_for(std::chrono::milliseconds(33 - elapsed.count()));
            }
            last_frame = now;

            triangleWindow->present(); // Swap buffers to display the rendered content
        }

        // Cleanup in the rendering thread
        triangleThreadRunning = false;
    });

}

void ModelPreviewer::CreateWindowVoxel(std::shared_ptr<Window> mainWindow, ModelVoxelizer* modelVox_)
{
    if (voxelThreadRunning)
    {
        return;
    }

    modelVox = modelVox_;

    // Create Triangle Window in the Main Thread
    voxelWindow = std::make_shared<Window>(mainWindow.get());
    if (!voxelWindow)
    {
        printf("FAILED TO CREATE MODEL TRIANGLE VIEW WINDOW!\n");
        return;
    }

    voxelThreadRunning = true;
    voxelThread = std::thread([this]() {
        voxelWindow->makeContextCurrent();

        printf("STARTING RENDER LOOP\n");

        //generate voxels
        modelVox->voxelizeModel();


        // Render Loop
        while (voxelThreadRunning && !glfwWindowShouldClose(voxelWindow->getGlfwWindowHandle())) {
            if (glfwWindowShouldClose(voxelWindow->getGlfwWindowHandle())) {
                voxelThreadRunning = false;
                break; 
            }

            voxelWindow->update();


            // Initial render
            RenderWindowVoxel();


            static auto last_frame = std::chrono::steady_clock::now();
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_frame);
            if (elapsed.count() < 33) {  // ~30 FPS
                std::this_thread::sleep_for(std::chrono::milliseconds(33 - elapsed.count()));
            }
            last_frame = now;

            voxelWindow->present(); // Swap buffers to display the rendered content
        }

        // Cleanup in the rendering thread
        voxelThreadRunning = false;

    });

}

void ModelPreviewer::RenderWindowTriangle()
{
    if (triangleWindow && loadedModel && !glfwWindowShouldClose(triangleWindow->getGlfwWindowHandle()))
    {
        triangleWindow->makeContextCurrent();
        glClearColor(0.2f, 0.2f, 0.2f, 0.2f);
        glEnable(GL_DEPTH_TEST); 
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear depth?


        // Camera Setup
        glm::mat4 view = glm::lookAt(Position, Position + Front, Up);
        glm::mat4 projection = glm::perspective(glm::radians(60.0f), (float)windowSize.x / (float)windowSize.y, 0.001f, 1000.0f);
        glm::mat4 model = glm::mat4(1.0f);

        glUniformMatrix4fv(glGetUniformLocation(triangleShader->ID, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(triangleShader->ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(triangleShader->ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        //// Material Settings for Phong Shader
        glm::vec3 diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
        glm::vec3 specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
        glm::vec3 emissiveColor = glm::vec3(1.0f, 1.0f, 1.0f);
        glUniform3fv(glGetUniformLocation(triangleShader->ID, "defaultMaterial.diffuseColor"),  1,  glm::value_ptr(diffuseColor));
        glUniform3fv(glGetUniformLocation(triangleShader->ID, "defaultMaterial.specularColor"), 1, glm::value_ptr(specularColor));
        glUniform3fv(glGetUniformLocation(triangleShader->ID, "defaultMaterial.emissiveColor"), 1, glm::value_ptr(emissiveColor));
        float materialSpecFactor = 0.0f;
        float materialEmisFactor = 0.0f;
        float materialShininess = 32.0f;
        glUniform1f(glGetUniformLocation(triangleShader->ID, "defaultMaterial.specularFactor"), materialSpecFactor);
        glUniform1f(glGetUniformLocation(triangleShader->ID, "defaultMaterial.emissiveFactor"), materialEmisFactor);
        glUniform1f(glGetUniformLocation(triangleShader->ID, "defaultMaterial.shininess"), materialShininess);

        loadedModel->Draw(*triangleShader);
    }
}

void ModelPreviewer::RenderWindowVoxel()
{
    if (voxelWindow && modelVox->isVoxelized && !glfwWindowShouldClose(voxelWindow->getGlfwWindowHandle()))
    {
        triangleWindow->makeContextCurrent();

        glClearColor(0.2f, 0.2f, 0.2f, 0.2f);
        glEnable(GL_DEPTH_TEST); 
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear depth?

        modelVox->DrawVoxels(*voxelShader, Position, Front, Up, windowSize.x, windowSize.y);
    }
}

void ModelPreviewer::CloseWindowTriangle()
{
    if (!triangleThreadRunning) return;

    modelVox = nullptr;
    //Signal thead to stop
    triangleThreadRunning = false;

    //Ensure window gets closed
    if (triangleWindow)
    {
        glfwSetWindowShouldClose(triangleWindow->getGlfwWindowHandle(), true);
    }

    // Wait for thread to finish
    if (triangleThread.joinable()) {
        triangleThread.join();
    }
}

void ModelPreviewer::CloseWindowVoxel()
{
    if (!voxelThreadRunning) return;

    modelVox = nullptr;
    //Signal thead to stop
    voxelThreadRunning = false;

    //Ensure window gets closed
    if (voxelWindow)
    {
        glfwSetWindowShouldClose(voxelWindow->getGlfwWindowHandle(), true);
    }

    // Wait for thread to finish
    if (voxelThread.joinable()) {
        voxelThread.join();
    }
}




