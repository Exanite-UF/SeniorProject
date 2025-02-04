// Include GLEW before OpenGL and GLFW
#include <GL/glew.h>

#include <GLFW/glfw3.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <imgui/imgui_stdlib.h>

#include <glm/common.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>
#include <unordered_map>
#include <unordered_set>

#include "InputManager.h"
#include "ShaderManager.h"
#include "VoxelRenderer.h"
#include "VoxelWorld.h"
#include "Window.h"

// WASD Space Shift = movement
// q = capture mouse
// f = fullscreen toggle
// e = progress through the noise's time
// t = change between types of noise
// scroll = change move speed
// CTRL + scroll = change noise fill

// Can be replaced with input manager's once integrated into main
bool invalidateMouse = true;

bool isWorkload = false; // View toggle
bool isRand2 = true; // Noise type toggle
float fillAmount = 0.6;
bool remakeNoise = false;

double noiseTime = 0;

GLuint raymarcherGraphicsProgram;
GLuint makeNoiseComputeProgram;
GLuint makeMipMapComputeProgram;
GLuint assignMaterialComputeProgram;

std::array<float, 3> getCamDir(float theta, float phi)
{
    return {
        std::cos(theta) * std::cos(phi), std::sin(theta) * std::cos(phi), std::sin(phi)
    };
}

std::array<float, 3> getForward(float theta, float phi)
{
    return {
        std::cos(theta), std::sin(theta), 0
    };
}

std::array<float, 3> getRight(float theta, float phi)
{
    return {
        std::sin(theta), -std::cos(theta), 0
    };
}

int main()
{
    VoxelRenderer::runStartupTests();
    VoxelRenderer::log("Starting Voxel Renderer");

    // Init GLFW
    if (!glfwInit())
    {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    // Create window
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4); // Request OpenGL 4.6
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // Use Core profile
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Block usage of deprecated APIs

    auto window = std::make_shared<Window>(); // TODO: Rename this to window and use it instead of the raw pointer once the Window class is implemented
    auto inputManager = window->inputManager; // TODO: Rename this to window and use it instead of the raw pointer once the Window class is implemented
    auto& shaderManager = ShaderManager::getManager();
    auto& input = inputManager->input;
    window->glfwWindowHandle = glfwCreateWindow(1024, 1024, "Voxel Renderer", nullptr, nullptr);
    auto* windowInstance = window->glfwWindowHandle;
    if (windowInstance == nullptr)
    {
        throw std::runtime_error("Failed to create window");
    }
    window->registerCallbacks();

    glfwGetWindowPos(windowInstance, &window->lastWindowX, &window->lastWindowY);
    glfwGetWindowSize(windowInstance, &window->lastWindowWidth, &window->lastWindowHeight);

    // Init GLEW
    glfwMakeContextCurrent(windowInstance);
    glewExperimental = true;
    if (glewInit() != GLEW_OK)
    {
        throw std::runtime_error("Failed to initialize GLEW");
    }

    // TODO: Consider refactoring this and other ImGui init/render loop/deinit code into separate class
    // Setup IMGUI context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls
    ImGui_ImplGlfw_InitForOpenGL(windowInstance, true);
    ImGui_ImplOpenGL3_Init();

    // Vertex array
    GLuint emptyVertexArray;
    glGenVertexArrays(1, &emptyVertexArray);

    // Get shader programs
    raymarcherGraphicsProgram = shaderManager.getGraphicsProgram("content/ScreenTri.vertex.glsl", "content/Raymarcher.fragment.glsl");
    makeNoiseComputeProgram = shaderManager.getComputeProgram("content/MakeNoise.compute.glsl");
    makeMipMapComputeProgram = shaderManager.getComputeProgram("content/MakeMipMap.compute.glsl");
    assignMaterialComputeProgram = shaderManager.getComputeProgram("content/AssignMaterial.compute.glsl");

    VoxelWorld voxelWorld(makeNoiseComputeProgram, makeMipMapComputeProgram, assignMaterialComputeProgram);
    VoxelRenderer renderer;
    Camera camera;
    renderer.setRaysPerPixel(1);

    std::vector<VoxelWorld> worlds;
    worlds.push_back(voxelWorld);
    worlds.push_back(voxelWorld);

    // Main render loop
    double theta = 0;
    double phi = 0;
    double camX = 0;
    double camY = 0;
    double camZ = 0;
    double mouseWheel = 0;

    glfwSwapInterval(0); // disable vsync
    glEnable(GL_FRAMEBUFFER_SRGB);
    glClearColor(0, 0, 0, 0);

    if (glfwRawMouseMotionSupported())
    {
        glfwSetInputMode(windowInstance, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }

    auto lastFrameTime = std::chrono::high_resolution_clock::now();

    worlds[1].position = glm::vec3(256, 0, 0);

    int counter = 0;
    double frameTime = 0;
    double totalTime = 0;
    while (!glfwWindowShouldClose(windowInstance))
    {
        auto now = std::chrono::high_resolution_clock::now();
        double deltaTime = std::chrono::duration<double>(now - lastFrameTime).count();
        lastFrameTime = now;
        frameTime += deltaTime;
        totalTime += deltaTime;

        counter++;
        if (counter % 10 == 0)
        {
            VoxelRenderer::log(std::to_string(10 / frameTime));
            frameTime = 0;
        }

        window->update();
        ImGui_ImplOpenGL3_NewFrame(); // TODO: Cleanup
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        int width, height;
        glfwGetWindowSize(windowInstance, &width, &height);
        renderer.setResolution(width, height);
        if (!invalidateMouse)
        {
            auto mouseDelta = input->getMouseDelta();

            theta -= mouseDelta.x * 0.002;
            phi += mouseDelta.y * 0.002;
            phi = std::min(std::max(phi, -3.1415926589 / 2), 3.1415926589 / 2);
        }
        else
        {
            invalidateMouse = false;
        }

        auto right = getRight(theta, phi);
        auto forward = getForward(theta, phi);
        auto camDirection = getCamDir(theta, phi);

        if (input->isKeyHeld(GLFW_KEY_A))
        {
            camX -= right[0] * deltaTime * std::pow(2, mouseWheel * 0.1);
            camY -= right[1] * deltaTime * std::pow(2, mouseWheel * 0.1);
            camZ -= right[2] * deltaTime * std::pow(2, mouseWheel * 0.1);
        }
        if (input->isKeyHeld(GLFW_KEY_D))
        {
            camX += right[0] * deltaTime * std::pow(2, mouseWheel * 0.1);
            camY += right[1] * deltaTime * std::pow(2, mouseWheel * 0.1);
            camZ += right[2] * deltaTime * std::pow(2, mouseWheel * 0.1);
        }

        if (input->isKeyHeld(GLFW_KEY_W))
        {
            camX += forward[0] * deltaTime * std::pow(2, mouseWheel * 0.1);
            camY += forward[1] * deltaTime * std::pow(2, mouseWheel * 0.1);
            camZ += forward[2] * deltaTime * std::pow(2, mouseWheel * 0.1);
        }
        if (input->isKeyHeld(GLFW_KEY_S))
        {
            camX -= forward[0] * deltaTime * std::pow(2, mouseWheel * 0.1);
            camY -= forward[1] * deltaTime * std::pow(2, mouseWheel * 0.1);
            camZ -= forward[2] * deltaTime * std::pow(2, mouseWheel * 0.1);
        }

        if (input->isKeyHeld(GLFW_KEY_SPACE))
        {
            camZ += deltaTime * std::pow(2, mouseWheel * 0.1);
        }
        if (input->isKeyHeld(GLFW_KEY_LEFT_SHIFT))
        {
            camZ -= deltaTime * std::pow(2, mouseWheel * 0.1);
        }

        if (input->isKeyHeld(GLFW_KEY_E))
        {
            voxelWorld.generateFromNoise(deltaTime, isRand2, fillAmount);
        }

        if (remakeNoise)
        {
            // The noise time should not be incremented here
            voxelWorld.generateFromNoise(0, isRand2, fillAmount);
            remakeNoise = false;
        }

        if (input->isKeyPressed(GLFW_KEY_F))
        {
            GLFWmonitor* monitor = glfwGetWindowMonitor(windowInstance);
            if (monitor == NULL)
            {
                window->toFullscreen();
            }
            else
            {
                window->toWindowed();
            }
        }

        if (input->isKeyPressed(GLFW_KEY_Q))
        {
            int mode = glfwGetInputMode(windowInstance, GLFW_CURSOR);

            if (mode == GLFW_CURSOR_DISABLED)
            {
                glfwSetInputMode(windowInstance, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
            else
            {
                glfwSetInputMode(windowInstance, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }
        }
        if (input->isKeyPressed(GLFW_KEY_ESCAPE))
        {
            glfwSetWindowShouldClose(windowInstance, GLFW_TRUE);
        }
        if (input->isKeyPressed(GLFW_KEY_R))
        {
            isWorkload = !isWorkload;
        }
        if (input->isKeyPressed(GLFW_KEY_T))
        {
            isRand2 = !isRand2;
            remakeNoise = true;
        }

        // Scroll
        if (input->isKeyHeld(GLFW_KEY_LEFT_CONTROL))
        {
            fillAmount -= input->getMouseScroll().y * 0.01;
            fillAmount = std::clamp(fillAmount, 0.f, 1.f);
            remakeNoise = true;
        }
        else
        {
            mouseWheel += input->getMouseScroll().y;
        }

        {
            camera.position = glm::vec3(camX, camY, camZ);
            worlds[0].orientation = glm::angleAxis((float)totalTime, glm::normalize(glm::vec3(-1.f, 0.5f, 1.f)));
            worlds[0].scale = glm::vec3(1, 1, 2);
            camera.orientation = glm::angleAxis((float)theta, glm::vec3(0.f, 0.f, 1.f)) * glm::angleAxis((float)phi, glm::vec3(0, 1, 0)); // glm::quatLookAt(glm::vec3(camDirection[0], camDirection[1], camDirection[2]), glm::vec3(1, 0, 0));
            renderer.prepareRayTraceFromCamera(camera);
            renderer.executeRayTrace(worlds);
            renderer.display();
        }

        /*
        {

            glUseProgram(raymarcherGraphicsProgram);
            glBindVertexArray(emptyVertexArray);

            voxelWorld.bindTextures();

            int camPos = glGetUniformLocation(raymarcherGraphicsProgram, "camPos");
            glUniform3f(camPos, camX, camY, camZ);

            int camDir = glGetUniformLocation(raymarcherGraphicsProgram, "camDir");
            glUniform3f(camDir, camDirection[0], camDirection[1], camDirection[2]);

            int resolution = glGetUniformLocation(raymarcherGraphicsProgram, "resolution");
            glUniform2f(resolution, width, height);

            glUniform1i(glGetUniformLocation(raymarcherGraphicsProgram, "isWorkload"), isWorkload);

            glDrawArrays(GL_TRIANGLES, 0, 3);

            glUseProgram(0);
            glBindVertexArray(0);
        }
        */

        {
            std::string str = "Hello";
            float f;
            ImGui::Text("Hello, world %d", 123);
            if (ImGui::Button("Save"))
                ;
            ImGui::InputText("string", &str);
            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(windowInstance);
    }

    // TODO: Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();

    return 0;
}
