#include <src/voxelizer/ModelPreviewer.h>
#include <thread>
#include <chrono>
#include <thread>


ModelPreviewer::~ModelPreviewer()
{
    if (loadedModel)
    {
        delete loadedModel;
        delete shader;
    }
    CloseWindowTriangle();
}

void ModelPreviewer::setModel(Model* model_)
{
    loadedModel = model_;
    shader = new Shader(vertShaderPath.c_str(), fragShaderPath.c_str());
}


void ModelPreviewer::CreateWindowTriangle(ModelVoxelizer* modelVox, std::string modelPath)
{
    if (triangleThreadRunning)
    {
        return;
    }

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
    triangleThread = std::thread([this, modelVox, modelPath]() {
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
            glfwPollEvents();

            // Initial render
            RenderWindowTriangle();
            glfwSwapBuffers(triangleWindow);


            static auto last_frame = std::chrono::steady_clock::now();
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_frame);
            if (elapsed.count() < 33) {  // ~30 FPS
                std::this_thread::sleep_for(std::chrono::milliseconds(33 - elapsed.count()));
            }
            last_frame = now;
        }

        // Cleanup in the rendering thread
        glfwDestroyWindow(triangleWindow);
        triangleWindow = nullptr;
        triangleThreadRunning = false;

    });

}

void ModelPreviewer::CreateWindowVoxel()
{
    // Create Triangle Window
    if (!voxelWindow)
    {
        voxelWindow = glfwCreateWindow(windowSize.x, windowSize.y, "Model Voxel View", NULL, NULL);
        if (!voxelWindow)
        {
            printf("FAILED TO CREATE MODEL VOXEL VIEW WINDOW!\n");
            return;
        }
    }
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

        shader->use();

        glUniformMatrix4fv(glGetUniformLocation(shader->ID, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(shader->ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shader->ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        //// Material Settings for Phong Shader
        glm::vec3 diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
        glm::vec3 specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
        glm::vec3 emissiveColor = glm::vec3(1.0f, 1.0f, 1.0f);
        glUniform3fv(glGetUniformLocation(shader->ID, "defaultMaterial.diffuseColor"),  1,  glm::value_ptr(diffuseColor));
        glUniform3fv(glGetUniformLocation(shader->ID, "defaultMaterial.specularColor"), 1, glm::value_ptr(specularColor));
        glUniform3fv(glGetUniformLocation(shader->ID, "defaultMaterial.emissiveColor"), 1, glm::value_ptr(emissiveColor));
        float materialSpecFactor = 0.0f;
        float materialEmisFactor = 0.0f;
        float materialShininess = 32.0f;
        glUniform1f(glGetUniformLocation(shader->ID, "defaultMaterial.specularFactor"), materialSpecFactor);
        glUniform1f(glGetUniformLocation(shader->ID, "defaultMaterial.emissiveFactor"), materialEmisFactor);
        glUniform1f(glGetUniformLocation(shader->ID, "defaultMaterial.shininess"), materialShininess);

        loadedModel->Draw(*shader);

        glfwSwapBuffers(triangleWindow);
    }
}

void ModelPreviewer::RenderWindowVoxel()
{
    if (voxelWindow)
    {
        glfwMakeContextCurrent(voxelWindow);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT); // clear depth?

        glfwSwapBuffers(voxelWindow);

        if (glfwWindowShouldClose(voxelWindow))
        {
            glfwDestroyWindow(voxelWindow);
            voxelWindow = nullptr;
        }
    }
}

void ModelPreviewer::CloseWindowTriangle()
{
    if (!triangleThreadRunning) return;

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
    if (voxelWindow)
    {
        glfwSetWindowShouldClose(voxelWindow, true);
    }
}




