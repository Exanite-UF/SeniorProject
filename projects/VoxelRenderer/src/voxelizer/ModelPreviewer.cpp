#include <src/voxelizer/ModelPreviewer.h>

ModelPreviewer::~ModelPreviewer()
{
    if (loadedModel)
    {
        delete loadedModel;
        delete shader;
    }
}

void ModelPreviewer::setModel(Model* model_)
{
    loadedModel = model_;
    shader = new Shader(vertShaderPath.c_str(), fragShaderPath.c_str());
}


void ModelPreviewer::CreateWindowTriangle()
{
    // Create Triangle Window
    if (!triangleWindow)
    {
        triangleWindow = glfwCreateWindow(windowSize.x, windowSize.y, "Model Triangle View", NULL, NULL);
        if (!triangleWindow)
        {
            printf("FAILED TO CREATE MODEL TRIANGLE VIEW WINDOW!\n");
            return;
        }
    }
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
    if (triangleWindow)
    {
        glfwMakeContextCurrent(triangleWindow);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT); // clear depth?

        // Camera Setup
        glm::mat4 view = glm::lookAt(Position, Position + Front, Up);
        glm::mat4 projection = glm::perspective(glm::radians(80.0f), (float)windowSize.x / (float)windowSize.y, 0.001f, 1000.0f);
        glm::mat4 model = glm::mat4(1.0f);

        shader->use();

        glUniformMatrix4fv(glGetUniformLocation(shader->ID, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(shader->ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shader->ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        // Material Settings for Phong Shader
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

        if (glfwWindowShouldClose(triangleWindow))
        {
            glfwDestroyWindow(triangleWindow);
            triangleWindow = nullptr;
        }
        std::cout << "YES" << std::endl;
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
    if (triangleWindow)
    {
        glfwSetWindowShouldClose(triangleWindow, true);
    }
}

void ModelPreviewer::CloseWindowVoxel()
{
    if (voxelWindow)
    {
        glfwSetWindowShouldClose(voxelWindow, true);
    }
}




