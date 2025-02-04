#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <imgui/imgui_stdlib.h>

#include <filesystem>
#include <iostream>
#include <string>

#include "ShaderManager.h"
#include "TupleHasher.h"
#include "VoxelRenderer.h"
#include "VoxelRendererProgram.h"
#include "VoxelWorld.h"
#include "Window.h"

VoxelRendererProgram::VoxelRendererProgram()
{
    // Ensure preconditions are met
    runStartupTests();
    log("Starting Voxel Renderer");

    // Init GLFW
    if (!glfwInit())
    {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    // TODO: This code should be part of the Window class
    // Create window
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4); // Request OpenGL 4.6
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // Use Core profile
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Block usage of deprecated APIs

    inputManager = std::make_shared<InputManager>();
    window = std::make_shared<Window>(inputManager);

    // TODO: This code should be part of the Window class
    window->glfwWindowHandle = glfwCreateWindow(1024, 1024, "Voxel Renderer", nullptr, nullptr);
    if (window->glfwWindowHandle == nullptr)
    {
        throw std::runtime_error("Failed to create window");
    }
    window->registerCallbacks();

    // Init GLEW
    glfwMakeContextCurrent(window->glfwWindowHandle);
    glewExperimental = true;
    if (glewInit() != GLEW_OK)
    {
        throw std::runtime_error("Failed to initialize GLEW");
    }

    // Init IMGUI
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls
    ImGui_ImplGlfw_InitForOpenGL(window->glfwWindowHandle, true);
    ImGui_ImplOpenGL3_Init();
}

VoxelRendererProgram::~VoxelRendererProgram()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
}

void VoxelRendererProgram::run()
{
    auto& shaderManager = ShaderManager::getManager();
    auto& input = inputManager->input;

    // TODO: This code should be part of the Window class
    glfwGetWindowPos(window->glfwWindowHandle, &window->lastWindowX, &window->lastWindowY);
    glfwGetWindowSize(window->glfwWindowHandle, &window->lastWindowWidth, &window->lastWindowHeight);

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
        glfwSetInputMode(window->glfwWindowHandle, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }

    auto lastFrameTime = std::chrono::high_resolution_clock::now();

    worlds[1].position = glm::vec3(256, 0, 0);

    int counter = 0;
    double frameTime = 0;
    double totalTime = 0;
    while (!glfwWindowShouldClose(window->glfwWindowHandle))
    {
        auto now = std::chrono::high_resolution_clock::now();
        double deltaTime = std::chrono::duration<double>(now - lastFrameTime).count();
        lastFrameTime = now;
        frameTime += deltaTime;
        totalTime += deltaTime;

        counter++;
        if (counter % 10 == 0)
        {
            VoxelRendererProgram::log(std::to_string(10 / frameTime));
            frameTime = 0;
        }

        // Update IMGUI
        ImGui_ImplOpenGL3_NewFrame(); // TODO: Cleanup
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Clear screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Update systems
        window->update();
        inputManager->update();

        int width, height;
        glfwGetWindowSize(window->glfwWindowHandle, &width, &height);
        renderer.setResolution(width, height);
        if (!inputManager->invalidateMouse)
        {
            auto mouseDelta = input->getMouseDelta();

            theta -= mouseDelta.x * 0.002;
            phi += mouseDelta.y * 0.002;
            phi = std::min(std::max(phi, -3.1415926589 / 2), 3.1415926589 / 2);
        }
        else
        {
            inputManager->invalidateMouse = false;
        }

        auto right = Camera::getRight(theta, phi);
        auto forward = Camera::getForward(theta, phi);
        auto camDirection = Camera::getCamDir(theta, phi);

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
            GLFWmonitor* monitor = glfwGetWindowMonitor(window->glfwWindowHandle);
            if (monitor == NULL)
            {
                window->setFullscreen();
            }
            else
            {
                window->setWindowed();
            }
        }

        if (input->isKeyPressed(GLFW_KEY_Q))
        {
            int mode = glfwGetInputMode(window->glfwWindowHandle, GLFW_CURSOR);

            if (mode == GLFW_CURSOR_DISABLED)
            {
                glfwSetInputMode(window->glfwWindowHandle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
            else
            {
                glfwSetInputMode(window->glfwWindowHandle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }
        }
        if (input->isKeyPressed(GLFW_KEY_ESCAPE))
        {
            glfwSetWindowShouldClose(window->glfwWindowHandle, GLFW_TRUE);
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

        glfwSwapBuffers(window->glfwWindowHandle);
    }
}

void VoxelRendererProgram::log(const std::string& value)
{
    std::cout << value + "\n"
              << std::flush;
}

void VoxelRendererProgram::checkForContentFolder()
{
    if (!std::filesystem::is_directory("content"))
    {
        throw std::runtime_error("Could not find content folder. Is the working directory set correctly?");
    }
    else
    {
        log("Found content folder");
    }
}

void VoxelRendererProgram::assertIsTrue(const bool condition, const std::string& errorMessage)
{
    if (!condition)
    {
        throw std::runtime_error("Assert failed: " + errorMessage);
    }
}

void VoxelRendererProgram::runStartupTests()
{
    log("Running startup tests");

    checkForContentFolder();
    assertIsTrue(NULL == nullptr, "Unsupported compiler: NULL must equal nullptr");

    auto key = std::make_tuple("hello", 1);
    std::unordered_map<std::tuple<std::string, int>, int, TupleHasher<std::tuple<std::string, int>>> map;
    assertIsTrue(!map.contains(key), "Incorrect map implementation: Map should be empty");
    map[key] = 1;
    assertIsTrue(map.contains(key), "Incorrect map implementation: Map should contain key that was inserted");
}
