#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <imgui/imgui_stdlib.h>

#include <filesystem>
#include <iostream>
#include <string>

#include "Event.h"
#include "ShaderManager.h"
#include "TupleHasher.h"
#include "VoxelRenderer.h"
#include "VoxelRendererProgram.h"
#include "VoxelWorld.h"
#include "Window.h"

void VoxelRendererProgram::onOpenGlDebugMessage(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
    std::string messageStr(message, length);
    log(messageStr);
}

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

    inputManager = std::make_shared<InputManager>();
    window = std::make_shared<Window>(inputManager);

    // Init IMGUI
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls
    ImGui_ImplGlfw_InitForOpenGL(window->glfwWindowHandle, true);
    ImGui_ImplOpenGL3_Init();

    // Init GLEW
    glewExperimental = true;
    if (glewInit() != GLEW_OK)
    {
        throw std::runtime_error("Failed to initialize GLEW");
    }

    // Attach debug message callback
    glDebugMessageCallback(VoxelRendererProgram::onOpenGlDebugMessage, this);
}

VoxelRendererProgram::~VoxelRendererProgram()
{
    // Shutdown IMGUI
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // Shutdown GLFW
    glfwTerminate();
}

void VoxelRendererProgram::run()
{
    {
        GLint size;
        glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &size);
        std::cout << "GL_MAX_SHADER_STORAGE_BLOCK_SIZE is " << (unsigned long long)size << " bytes." << std::endl;
    }

    auto& shaderManager = ShaderManager::getManager();
    auto& input = inputManager->input;

    // Configure OpenGL
    glEnable(GL_FRAMEBUFFER_SRGB);
    glClearColor(0, 0, 0, 0);

    // Vertex array
    GLuint emptyVertexArray;
    glGenVertexArrays(1, &emptyVertexArray);

    // Get shader programs
    raymarcherGraphicsProgram = shaderManager.getGraphicsProgram("content/ScreenTri.vertex.glsl", "content/Raymarcher.fragment.glsl");
    makeNoiseComputeProgram = shaderManager.getComputeProgram("content/MakeNoise.compute.glsl");
    makeMipMapComputeProgram = shaderManager.getComputeProgram("content/MakeMipMap.compute.glsl");
    assignMaterialComputeProgram = shaderManager.getComputeProgram("content/AssignMaterial.compute.glsl");

    // Voxel rendering
    VoxelWorld voxelWorld(makeNoiseComputeProgram, makeMipMapComputeProgram, assignMaterialComputeProgram);
    VoxelRenderer renderer;
    Camera camera;
    renderer.setRaysPerPixel(1);

    std::vector<VoxelWorld> worlds;
    worlds.push_back(voxelWorld);
    // worlds.push_back(voxelWorld);

    // worlds[1].position = glm::vec3(256, 0, 0);

    // Main render loop
    glm::vec3 cameraPosition(0);
    glm::vec2 cameraRotation(0);
    float moveSpeed = 0;
    float mouseSensitivity = 0.002;

    // Engine time
    double totalTime = 0;
    auto previousTimestamp = std::chrono::high_resolution_clock::now();

    // Fps counter
    int frameCounter = 0;
    double frameTime = 0;
    while (!glfwWindowShouldClose(window->glfwWindowHandle))
    {
        // Engine time
        auto now = std::chrono::high_resolution_clock::now();
        double deltaTime = std::chrono::duration<double>(now - previousTimestamp).count();
        previousTimestamp = now;
        frameTime += deltaTime;
        totalTime += deltaTime;

        // Fps counter
        frameCounter++;
        if (frameCounter % 10 == 0)
        {
            // log(std::to_string(10 / frameTime));
            frameTime = 0;
        }

        // Clear screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Update IMGUI
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Update systems
        window->update();
        inputManager->update();

        if (!inputManager->cursorEnteredThisFrame)
        {
            auto mouseDelta = input->getMouseDelta();

            cameraRotation.y -= mouseDelta.x * mouseSensitivity;
            cameraRotation.x += mouseDelta.y * mouseSensitivity;
            cameraRotation.x = std::min(std::max(cameraRotation.x, -glm::pi<float>() / 2), glm::pi<float>() / 2);
        }
        else
        {
            inputManager->cursorEnteredThisFrame = false;
        }

        auto right = Camera::getRight(cameraRotation.y, cameraRotation.x);
        auto forward = Camera::getForward(cameraRotation.y, cameraRotation.x);

        if (input->isKeyHeld(GLFW_KEY_A))
        {
            cameraPosition -= static_cast<float>(deltaTime * std::pow(2, moveSpeed * 0.1)) * right;
        }

        if (input->isKeyHeld(GLFW_KEY_D))
        {
            cameraPosition += static_cast<float>(deltaTime * std::pow(2, moveSpeed * 0.1)) * right;
        }

        if (input->isKeyHeld(GLFW_KEY_W))
        {
            cameraPosition += static_cast<float>(deltaTime * std::pow(2, moveSpeed * 0.1)) * forward;
        }

        if (input->isKeyHeld(GLFW_KEY_S))
        {
            cameraPosition -= static_cast<float>(deltaTime * std::pow(2, moveSpeed * 0.1)) * forward;
        }

        if (input->isKeyHeld(GLFW_KEY_SPACE))
        {
            cameraPosition.z += static_cast<float>(deltaTime * std::pow(2, moveSpeed * 0.1));
        }

        if (input->isKeyHeld(GLFW_KEY_LEFT_SHIFT))
        {
            cameraPosition.z -= static_cast<float>(deltaTime * std::pow(2, moveSpeed * 0.1));
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
            moveSpeed += input->getMouseScroll().y;
        }

        {
            renderer.setResolution(window->size.x, window->size.y);

            camera.position = cameraPosition;
            // worlds[0].rotation = glm::angleAxis((float)totalTime, glm::normalize(glm::vec3(-1.f, 0.5f, 1.f)));
            // worlds[0].scale = glm::vec3(1, 1, 2);
            camera.rotation = glm::angleAxis((float)cameraRotation.y, glm::vec3(0.f, 0.f, 1.f)) * glm::angleAxis((float)cameraRotation.x, glm::vec3(0, 1, 0)); // glm::quatLookAt(glm::vec3(camDirection[0], camDirection[1], camDirection[2]), glm::vec3(1, 0, 0));

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

    log("Found content folder");
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

    {
        // Make sure the content folder exists
        checkForContentFolder();
    }

    {
        // This allows us to safely use nullptr instead of NULL
        assertIsTrue(NULL == nullptr, "Unsupported compiler: NULL must equal nullptr");
    }

    {
        // A tuple can be used as a key in a map
        // Must make sure to provide the TupleHasher template argument
        auto key = std::make_tuple("hello", 1);
        std::unordered_map<std::tuple<std::string, int>, int, TupleHasher<std::tuple<std::string, int>>> map;
        assertIsTrue(!map.contains(key), "Incorrect map implementation: Map should be empty");
        map[key] = 1;
        assertIsTrue(map.contains(key), "Incorrect map implementation: Map should contain key that was inserted");
    }

    {
        // This declares an event with a single int parameter
        Event<int> testEvent;
        int counter = 0;

        // Subscribing returns a shared_ptr that acts as a subscription handle
        // Letting the shared_ptr fall out of scope or explicitly resetting it will unsubscribe from the event
        auto listener = testEvent.subscribe([&](int value)
            {
                log("Event was successfully called");
                counter += value;
            });

        // Listener is subscribed, this should add 5 to the counter
        testEvent.raise(5);
        assertIsTrue(counter == 5, "Incorrect event implementation: counter should equal 5");

        // Unsubscribe from event
        listener.reset();

        // Listener is unsubscribed, this should not affect the counter
        testEvent.raise(5);
        assertIsTrue(counter == 5, "Incorrect event implementation: counter should equal 5");
    }
}
