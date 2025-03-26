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
}

void ModelPreviewer::setModel(Model* model_)
{
    loadedModel = model_;
    triangleShader = new Shader(vertShaderPathTriangle.c_str(), fragShaderPathTriangle.c_str());
    voxelShader = new Shader(vertShaderPathVoxel.c_str(), fragShaderPathVoxel.c_str());
}


void ModelPreviewer::CreateWindowTriangle(ModelVoxelizer* modelVox_, std::string modelPath)
{
    if (triangleThreadRunning)
    {
        return;
    }

    modelVox = modelVox_;

    // Ensure GLFW is initialized in the main thread
    if (!glfwInit())
    {
        printf("FAILED TO INIT GLFW!\n");
        return;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create Triangle Window in the Main Thread
    triangleWindow = glfwCreateWindow(windowSize.x, windowSize.y, "Model Triangle View", nullptr, NULL);
    if (!triangleWindow)
    {
        printf("FAILED TO CREATE MODEL TRIANGLE VIEW WINDOW!\n");
        return;
    }

    triangleThreadRunning = true;
    triangleThread = std::thread([this, modelPath]() {
        glfwMakeContextCurrent(triangleWindow);

        modelVox->loadModel(const_cast<char*>(modelPath.c_str()));
        setModel(modelVox->getModel());

        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK)
        {
            printf("FAILED TO INITIALIZE GLEW!\n");
            triangleThreadRunning = false;
            return;
        }


        // Render Loop
        while (triangleThreadRunning && !glfwWindowShouldClose(triangleWindow)) {
            if (glfwWindowShouldClose(triangleWindow)) {
                triangleThreadRunning = false;
                break; 
            }
            glfwMakeContextCurrent(triangleWindow);
            glfwPollEvents();

            // Initial render
            RenderWindowTriangle();


            static auto last_frame = std::chrono::steady_clock::now();
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_frame);
            if (elapsed.count() < 33) {  // ~30 FPS
                std::this_thread::sleep_for(std::chrono::milliseconds(33 - elapsed.count()));
            }
            last_frame = now;

            glfwSwapBuffers(triangleWindow);
        }

        // Cleanup in the rendering thread
        glfwDestroyWindow(triangleWindow);
        triangleWindow = nullptr;
        triangleThreadRunning = false;

    });

}

void ModelPreviewer::CreateWindowVoxel(ModelVoxelizer* modelVox_)
{
    if (voxelThreadRunning)
    {
        return;
    }

    modelVox = modelVox_;

    // Ensure GLFW is initialized in the main thread
    if (!glfwInit())
    {
        printf("FAILED TO INIT GLFW!\n");
        return;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create Triangle Window in the Main Thread
    voxelWindow = glfwCreateWindow(windowSize.x, windowSize.y, "Model Voxel View", nullptr, NULL);
    if (!voxelWindow)
    {
        printf("FAILED TO CREATE MODEL TRIANGLE VIEW WINDOW!\n");
        return;
    }

    voxelThreadRunning = true;
    voxelThread = std::thread([this]() {
        glfwMakeContextCurrent(voxelWindow);



        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK)
        {
            printf("FAILED TO INITIALIZE GLEW!\n");
            voxelThreadRunning = false;
            return;
        }

        printf("STARTING RENDER LOOP\n");

        //generate voxels
        modelVox->voxelizeModel();


        // Render Loop
        while (voxelThreadRunning && !glfwWindowShouldClose(voxelWindow)) {
            if (glfwWindowShouldClose(voxelWindow)) {
                voxelThreadRunning = false;
                break; 
            }

            glfwPollEvents();

            // Initial render
            RenderWindowVoxel();
            glfwSwapBuffers(voxelWindow);


            static auto last_frame = std::chrono::steady_clock::now();
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_frame);
            if (elapsed.count() < 33) {  // ~30 FPS
                std::this_thread::sleep_for(std::chrono::milliseconds(33 - elapsed.count()));
            }
            last_frame = now;
        }

        // Cleanup in the rendering thread
        glfwDestroyWindow(voxelWindow);
        voxelWindow = nullptr;
        voxelThreadRunning = false;

    });

}

void ModelPreviewer::RenderWindowTriangle()
{
    if (triangleWindow && loadedModel && !glfwWindowShouldClose(triangleWindow))
    {
        glfwMakeContextCurrent(triangleWindow);
        glClearColor(0.2f, 0.2f, 0.2f, 0.2f);
        glEnable(GL_DEPTH_TEST); 
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear depth?

        // Camera Setup
        glm::mat4 view = glm::lookAt(Position, Position + Front, Up);
        glm::mat4 projection = glm::perspective(glm::radians(60.0f), (float)windowSize.x / (float)windowSize.y, 0.001f, 1000.0f);
        glm::mat4 model = glm::mat4(1.0f);

        triangleShader->use();

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

        glfwSwapBuffers(triangleWindow);
    }
}

void ModelPreviewer::RenderWindowVoxel()
{
    if (voxelWindow && modelVox->isVoxelized && !glfwWindowShouldClose(voxelWindow))
    {
        glfwMakeContextCurrent(voxelWindow);
        glClearColor(0.2f, 0.2f, 0.2f, 0.2f);
        glEnable(GL_DEPTH_TEST); 
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear depth?

        // Camera Setup
        glm::mat4 view = glm::lookAt(Position, Position + Front, Up);
        glm::mat4 projection = glm::perspective(glm::radians(60.0f), (float)windowSize.x / (float)windowSize.y, 0.001f, 1000.0f);
        glm::mat4 model = glm::mat4(1.0f);

        voxelShader->use();

        glUniformMatrix4fv(glGetUniformLocation(voxelShader->ID, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(voxelShader->ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(voxelShader->ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        //// Material Settings for Phong Shader
        glm::vec3 diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
        glm::vec3 specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
        glm::vec3 emissiveColor = glm::vec3(1.0f, 1.0f, 1.0f);
        glUniform3fv(glGetUniformLocation(voxelShader->ID, "defaultMaterial.diffuseColor"),  1,  glm::value_ptr(diffuseColor));
        glUniform3fv(glGetUniformLocation(voxelShader->ID, "defaultMaterial.specularColor"), 1, glm::value_ptr(specularColor));
        glUniform3fv(glGetUniformLocation(voxelShader->ID, "defaultMaterial.emissiveColor"), 1, glm::value_ptr(emissiveColor));
        float materialSpecFactor = 0.0f;
        float materialEmisFactor = 0.0f;
        float materialShininess = 32.0f;
        glUniform1f(glGetUniformLocation(voxelShader->ID, "defaultMaterial.specularFactor"), materialSpecFactor);
        glUniform1f(glGetUniformLocation(voxelShader->ID, "defaultMaterial.emissiveFactor"), materialEmisFactor);
        glUniform1f(glGetUniformLocation(voxelShader->ID, "defaultMaterial.shininess"), materialShininess);

        //loadedModel->Draw(*shader);
        modelVox->DrawVoxels(*voxelShader);

        glfwSwapBuffers(voxelWindow);
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
        glfwSetWindowShouldClose(triangleWindow, true);
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
        glfwSetWindowShouldClose(voxelWindow, true);
    }

    // Wait for thread to finish
    if (voxelThread.joinable()) {
        voxelThread.join();
    }
}




