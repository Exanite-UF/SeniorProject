#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <imgui/imgui_stdlib.h>

#include <Jolt/Jolt.h>

#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#define STB_IMAGE_IMPLEMENTATION
#include "graphics/TextureManager.h"

#include <stb_image.h>

#include <filesystem>
#include <iostream>
#include <string>

#include <src/Content.h>
#include <src/Program.h>
#include <src/graphics/ShaderManager.h>
#include <src/rendering/VoxelRenderer.h>
#include <src/utilities/Assert.h>
#include <src/utilities/BufferedEvent.h>
#include <src/utilities/Event.h>
#include <src/utilities/Log.h>
#include <src/utilities/TupleHasher.h>
#include <src/windowing/Window.h>
#include <src/world/MaterialManager.h>
#include <src/world/Scene.h>
#include <src/world/VoxelWorld.h>
#include <src/world/VoxelWorldData.h>

void Program::onOpenGlDebugMessage(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
    {
        return;
    }

    std::string messageStr(message, length);
    Log::log(messageStr);
}

Program::Program()
{
    // Ensure preconditions are met
    runEarlyStartupTests();
    Log::log("Starting Voxel Renderer");

    // Init GLFW
    if (!glfwInit())
    {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    window = std::make_shared<Window>();
    inputManager = std::make_shared<InputManager>(window);

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
    glDebugMessageCallback(Program::onOpenGlDebugMessage, this);
}

Program::~Program()
{
    // Shutdown IMGUI
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // Shutdown GLFW
    glfwTerminate();
}

void Program::run()
{
    // Ensure preconditions are met
    runLateStartupTests();

    auto& shaderManager = ShaderManager::getInstance();
    auto& textureManager = TextureManager::getInstance();
    auto& materialManager = MaterialManager::getInstance();
    auto& input = inputManager->input;

    // Configure OpenGL
    glEnable(GL_FRAMEBUFFER_SRGB);
    glClearColor(0, 0, 0, 0);

    // Load shader programs
    raymarcherGraphicsProgram = shaderManager.getGraphicsProgram(Content::screenTriVertexShader, Content::raymarcherFragmentShader);
    makeNoiseComputeProgram = shaderManager.getComputeProgram(Content::makeNoiseComputeShader);
    makeMipMapComputeProgram = shaderManager.getComputeProgram(Content::makeMipMapComputeShader);
    assignMaterialComputeProgram = shaderManager.getComputeProgram(Content::assignMaterialComputeShader);

    // Load textures
    // TODO: Remove OR save to Program class, similarly to the shaders above. These are used to make sure the texture loading is working
    auto texture = textureManager.loadTexture(Content::defaultColorTexture, ColorAlpha);
    auto texture1 = textureManager.loadTexture(Content::defaultColorTexture, ColorOnly);
    auto texture2 = textureManager.loadTexture(Content::defaultNormalTexture, Normal);

    // Create the scene
    Scene scene {};
    Camera& camera = scene.camera;

    scene.worlds.reserve(2); // Issue, this cannot have reallocations.
    scene.worlds.emplace_back(makeNoiseComputeProgram, makeMipMapComputeProgram, assignMaterialComputeProgram);
    // scene.worlds.emplace_back(makeNoiseComputeProgram, makeMipMapComputeProgram, assignMaterialComputeProgram);
    // scene.worlds.at(1).transform.addGlobalPosition(glm::vec3(256, 0, 0));

    VoxelWorld& voxelWorld = scene.worlds.at(0);

    VoxelWorldData data {};
    data.copyFrom(voxelWorld);

    // Create the renderer
    VoxelRenderer renderer;
    renderer.setRaysPerPixel(1);

    // Main render loop
    glm::vec3 cameraPosition(0);
    glm::vec2 cameraRotation(0);
    float moveSpeedExponent = 50;
    float mouseSensitivity = 0.002;

    // Engine time
    double totalElapsedTime = 0;
    auto previousTimestamp = std::chrono::high_resolution_clock::now();

    // Fps counter
    float fpsCycleTimer = 0;
    int framesThisCycle = 0;
    float currentFPS = 0;
    float averagedDeltaTime = 0;

    int frameCount = 0;
    int maxFrames = 0;

    // IMGUI Menu
    bool showMenuGUI = false;
    ImGuiWindowFlags guiWindowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    while (!glfwWindowShouldClose(window->glfwWindowHandle))
    {
        // Engine time
        auto currentTimestamp = std::chrono::high_resolution_clock::now();
        double deltaTime = std::chrono::duration<double>(currentTimestamp - previousTimestamp).count();
        previousTimestamp = currentTimestamp;
        totalElapsedTime += deltaTime;

        // Fps counter
        fpsCycleTimer += deltaTime;
        framesThisCycle++;
        if (fpsCycleTimer > 1)
        {
            currentFPS = framesThisCycle / fpsCycleTimer;
            averagedDeltaTime = fpsCycleTimer / framesThisCycle;

            auto averagedDeltaTimeMs = averagedDeltaTime * 1000;
            Log::log(std::to_string(currentFPS) + " FPS (" + std::to_string(averagedDeltaTimeMs) + " ms)");

            fpsCycleTimer = 0;
            framesThisCycle = 0;
        }

        // Clear screen
        // TODO: Why is this so far from the rest of the rendering code?
        if (maxFrames == 0 || frameCount < maxFrames)
        {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }

        // Update IMGUI
        ImGuiIO& io = ImGui::GetIO();
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

            if (glm::length(mouseDelta) > 0)
            {
                frameCount = 0;
            }
        }
        else
        {
            inputManager->cursorEnteredThisFrame = false;
        }

        auto right = Camera::getRight(cameraRotation.y, cameraRotation.x);
        auto forward = Camera::getForward(cameraRotation.y, cameraRotation.x);

        if (input->isKeyHeld(GLFW_KEY_A))
        {
            cameraPosition -= static_cast<float>(deltaTime * std::pow(2, moveSpeedExponent * 0.1)) * right;
            frameCount = 0;
        }

        if (input->isKeyHeld(GLFW_KEY_D))
        {
            cameraPosition += static_cast<float>(deltaTime * std::pow(2, moveSpeedExponent * 0.1)) * right;
            frameCount = 0;
        }

        if (input->isKeyHeld(GLFW_KEY_W))
        {
            cameraPosition += static_cast<float>(deltaTime * std::pow(2, moveSpeedExponent * 0.1)) * forward;
            frameCount = 0;
        }

        if (input->isKeyHeld(GLFW_KEY_S))
        {
            cameraPosition -= static_cast<float>(deltaTime * std::pow(2, moveSpeedExponent * 0.1)) * forward;
            frameCount = 0;
        }

        if (input->isKeyHeld(GLFW_KEY_SPACE))
        {
            cameraPosition.z += static_cast<float>(deltaTime * std::pow(2, moveSpeedExponent * 0.1));
            frameCount = 0;
        }

        if (input->isKeyHeld(GLFW_KEY_LEFT_SHIFT))
        {
            cameraPosition.z -= static_cast<float>(deltaTime * std::pow(2, moveSpeedExponent * 0.1));
            frameCount = 0;
        }

        if (input->isKeyHeld(GLFW_KEY_E))
        {
            voxelWorld.generateOccupancyAndMipMapsAndMaterials(deltaTime, isRand2, fillAmount);
        }

        if (input->isKeyPressed(GLFW_KEY_F5))
        {
            data.copyFrom(voxelWorld);
        }

        if (input->isKeyPressed(GLFW_KEY_F8))
        {
            data.clearOccupancy();

            for (int x = 0; x < data.getSize().x; ++x)
            {
                for (int y = 0; y < data.getSize().y; ++y)
                {
                    data.setVoxelOccupancy({ x, y, x }, true);
                }
            }

            data.writeTo(voxelWorld);
        }

        if (input->isKeyPressed(GLFW_KEY_F9))
        {
            data.writeTo(voxelWorld);
        }

        if (remakeNoise)
        {
            // The noise time should not be incremented here
            voxelWorld.generateOccupancyAndMipMapsAndMaterials(0, isRand2, fillAmount);
            remakeNoise = false;
        }

        if (input->isKeyPressed(GLFW_KEY_F))
        {
            GLFWmonitor* monitor = glfwGetWindowMonitor(window->glfwWindowHandle);
            if (monitor == NULL)
            {
                window->setFullscreen();
                frameCount = 0;
            }
            else
            {
                window->setWindowed();
                frameCount = 0;
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
            frameCount = 0;
        }
        else
        {
            moveSpeedExponent += input->getMouseScroll().y;
        }

        // F3 Debug Menu
        if (input->isKeyPressed(GLFW_KEY_F3))
        {
            showMenuGUI = !showMenuGUI;
        }

        if (maxFrames <= 0 || frameCount < maxFrames)
        {
            frameCount++;
            renderer.setResolution(window->size.x, window->size.y);

            camera.transform.setGlobalPosition(cameraPosition);
            camera.transform.setGlobalRotation(glm::angleAxis((float)cameraRotation.y, glm::vec3(0.f, 0.f, 1.f)) * glm::angleAxis((float)cameraRotation.x, glm::vec3(0, 1, 0)));

            // Scales and rotates the world. For testing purposes.
            // scene.worlds[0].transform.setLocalRotation(glm::angleAxis((float)1, glm::normalize(glm::vec3(1.f, 0.f, 0.0f))));
            // scene.worlds[0].transform.setLocalScale(glm::vec3(1, 1, 2));
            renderer.prepareRayTraceFromCamera(camera, frameCount == 1);
            for (int i = 0; i <= 2; i++)
            {
                renderer.executeRayTrace(scene.worlds, MaterialManager::getInstance());
            }

            renderer.display(camera, frameCount);
        }

        {
            if (showMenuGUI)
            {
                ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.4f));
                ImGui::SetNextWindowPos(ImVec2(0, 0)); // Set Menu to Top Left of Screen
                ImGui::Begin("Menu", nullptr, guiWindowFlags);
                {
                    ImGui::SetWindowFontScale(1.5f);
                    ImGui::Text("Voxel Rendering Project\n");
                    ImGui::SetWindowFontScale(1.0f);
                    ImGui::Text("Controls:");
                    ImGui::Text("\tW - Forward");
                    ImGui::Text("\tS - Backward");
                    ImGui::Text("\tA - Left");
                    ImGui::Text("\tD - Right");
                    ImGui::Text("\tEsc - Close Application");
                    ImGui::Text("\tE - Iterate Noise Over Time");
                    ImGui::Text("\tF - Toggle Fullscreen");
                    ImGui::Text("\tQ - Toggle Mouse Input");
                    ImGui::Text("\tT - Change Noise Type");
                    ImGui::Text("\tMouse Scroll - Change Move Speed");
                    ImGui::Text("\tCtrl + Mouse Scroll - Change Noise Fill");
                    ImGui::Text("\nCamera Position");
                    ImGui::Text("\tX: %.2f Y: %.2f Z: %.2f", cameraPosition.x, cameraPosition.y, cameraPosition.z);
                    ImGui::Text("\nCamera Look Direction");
                    ImGui::Text("\tX: %.2f Y: %.2f Z: %.2f", forward.x, forward.y, forward.z);
                    ImGui::Text("\nFPS: %.2f", currentFPS);
                    ImGui::Text("\nWindow Resolution: %.0f x %.0f", io.DisplaySize.x, io.DisplaySize.y);
                }
                ImGui::End();
                ImGui::PopStyleColor();
            }
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window->glfwWindowHandle);
    }
}

void Program::checkForContentFolder()
{
    if (!std::filesystem::is_directory("content"))
    {
        throw std::runtime_error("Could not find content folder. Is the working directory set correctly?");
    }

    Log::log("Found content folder");
}

void Program::runEarlyStartupTests()
{
    Log::log("Running early startup tests (in constructor)");

    {
        // Make sure the content folder exists
        checkForContentFolder();
    }

    {
        // This allows us to safely use nullptr instead of NULL
        Assert::isTrue(NULL == nullptr, "Unsupported compiler: NULL must equal nullptr");
    }

    {
        // This allows us to safely use uint32_t instead of GLuint
        Assert::isTrue(sizeof(GLuint) == sizeof(uint32_t), "Unsupported compiler: sizeof(GLuint) must equal sizeof(uint32_t)");
    }

    {
        // A tuple can be used as a key in a map
        // Must make sure to provide the TupleHasher template argument
        auto key = std::make_tuple("hello", 1);
        std::unordered_map<std::tuple<std::string, int>, int, TupleHasher<std::tuple<std::string, int>>> map;
        Assert::isTrue(!map.contains(key), "Incorrect map implementation: Map should be empty");
        map[key] = 1;
        Assert::isTrue(map.contains(key), "Incorrect map implementation: Map should contain key that was inserted");
    }

    {
        // This declares an event with a single int parameter
        Event<int> testEvent;
        int counter = 0;

        // Subscribing returns a shared_ptr that acts as a subscription handle
        // Letting the shared_ptr fall out of scope or explicitly resetting it will unsubscribe from the event
        auto listener = testEvent.subscribe([&](int value)
            {
                Log::log("Event was successfully called");
                counter += value;
            });

        // Listener is subscribed, this should add 5 to the counter
        testEvent.raise(5);
        Assert::isTrue(counter == 5, "Incorrect event implementation: counter should equal 5");

        // Unsubscribe from event
        listener.reset();

        // Listener is unsubscribed, this should not affect the counter
        testEvent.raise(5);
        Assert::isTrue(counter == 5, "Incorrect event implementation: counter should equal 5");
    }

    {
        // Similar to before, but events are buffered when raised and only sent when sendBufferedEvents is called
        BufferedEvent<int> testEvent;
        int counter = 0;

        auto listener = testEvent.subscribe([&](int value)
            {
                Log::log("Buffered event was successfully called");
                counter += value;
            });

        // Listener is subscribed, but event has not been flushed. This should not affect the counter
        testEvent.raise(5);
        Assert::isTrue(counter == 0, "Incorrect buffered event implementation: counter should equal 0");

        // Flush events, this should add 5 to the counter
        testEvent.flush();
        Assert::isTrue(counter == 5, "Incorrect buffered event implementation: counter should equal 5");

        // Unsubscribe from event
        listener.reset();

        // Listener is unsubscribed, this should not affect the counter
        testEvent.raise(5);
        testEvent.flush();
        Assert::isTrue(counter == 5, "Incorrect buffered event implementation: counter should equal 5");
    }
}

void Program::runLateStartupTests()
{
    Log::log("Running late startup tests (in run())");

    {
        // Verify shader storage block size is large enough
        GLint size;
        glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &size);
        Log::log("GL_MAX_SHADER_STORAGE_BLOCK_SIZE is " + std::to_string(size) + " bytes.");

        // 134217728 is the GL_MAX_SHADER_STORAGE_BLOCK_SIZE of Exanite's laptop, also equal to 512x512x512
        Assert::isTrue(size >= 134217728, "OpenGL driver not supported: GL_MAX_SHADER_STORAGE_BLOCK_SIZE is not big enough");
    }

    {
        // Verify shader storage block size is large enough
        glm::ivec3 workgroupSizes;
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &workgroupSizes.x);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &workgroupSizes.y);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &workgroupSizes.z);

        Log::log("GL_MAX_COMPUTE_WORK_GROUP_SIZE is <" + std::to_string(workgroupSizes.x) + ", " + std::to_string(workgroupSizes.y) + ", " + std::to_string(workgroupSizes.z) + ">" + ".");

        glm::ivec3 workgroupCounts;
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &workgroupCounts.x);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &workgroupCounts.y);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &workgroupCounts.z);

        Log::log("GL_MAX_COMPUTE_WORK_GROUP_COUNT is <" + std::to_string(workgroupCounts.x) + ", " + std::to_string(workgroupCounts.y) + ", " + std::to_string(workgroupCounts.z) + ">" + ".");
    }

    {
        // TODO: Probably remove later
        // Ensure assimp and jolt work
        JPH::RegisterDefaultAllocator();
        JPH::Factory::sInstance = new JPH::Factory();
        JPH::RegisterTypes();

        Assimp::Importer importer {};
        const aiScene* scene = importer.ReadFile("content/Cube.fbx", 0);
        importer.FreeScene();
    }
}
