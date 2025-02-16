#include <filesystem>
#include <iostream>
#include <string>

#include <src/Content.h>
#include <src/Program.h>
#include <src/graphics/ShaderManager.h>
#include <src/rendering/VoxelRenderer.h>
#include <src/utilities/BufferedEvent.h>
#include <src/utilities/Event.h>
#include <src/utilities/TupleHasher.h>
#include <src/windowing/Window.h>
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
    log(messageStr);
}

Program::Program()
{
    // Ensure preconditions are met
    runEarlyStartupTests();
    log("Starting Voxel Renderer");

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

    auto& shaderManager = ShaderManager::getManager();
    auto& input = inputManager->input;

    // Configure OpenGL
    glEnable(GL_FRAMEBUFFER_SRGB);
    glClearColor(0, 0, 0, 0);

    // Get shader programs
    raymarcherGraphicsProgram = shaderManager.getGraphicsProgram(Content::screenTriVertexShader, Content::raymarcherFragmentShader);
    makeNoiseComputeProgram = shaderManager.getComputeProgram(Content::makeNoiseComputeShader);
    makeMipMapComputeProgram = shaderManager.getComputeProgram(Content::makeMipMapComputeShader);
    assignMaterialComputeProgram = shaderManager.getComputeProgram(Content::assignMaterialComputeShader);

    // Create the scene
    Scene scene {};
    Camera& camera = scene.camera;

    VoxelWorld& voxelWorld = scene.worlds.emplace_back(makeNoiseComputeProgram, makeMipMapComputeProgram, assignMaterialComputeProgram);
    // worlds.at(1).transform.position = glm::vec3(256, 0, 0);

    VoxelWorldData data {};
    data.copyFrom(voxelWorld);

    // Create the renderer
    VoxelRenderer renderer;
    renderer.setRaysPerPixel(1);

    // Main render loop
    glm::vec3 cameraPosition(0);
    glm::vec2 cameraRotation(0);
    float moveSpeed = 0;
    float mouseSensitivity = 0.002;

    // Engine time
    double totalElapsedTime = 0;
    auto previousTimestamp = std::chrono::high_resolution_clock::now();

    // Fps counter
    int frameCounter = 0;
    double frameTime = 0;
    float currentFPS = 0;

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
        frameCounter++;
        frameTime += deltaTime;
        if (frameCounter % 100 == 0)
        {
            currentFPS = 100 / frameTime;
            log(std::to_string(currentFPS));
            frameTime = 0;
        }

        // Clear screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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
            voxelWorld.generateOccupancyAndMipMapsAndMaterials(deltaTime, isRand2, fillAmount);
        }

        if (input->isKeyPressed(GLFW_KEY_F5))
        {
            data.copyFrom(voxelWorld);
        }

        if (input->isKeyPressed(GLFW_KEY_F9))
        {
            data.writeTo(voxelWorld);
            data.clearOccupancy();
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

        // F3 Debug Menu
        if (input->isKeyPressed(GLFW_KEY_F3))
        {
            showMenuGUI = !showMenuGUI;
        }

        {
            renderer.setResolution(window->size.x, window->size.y);

            camera.transform.setGlobalPosition(cameraPosition);
            camera.transform.setGlobalRotation(glm::angleAxis((float)cameraRotation.y, glm::vec3(0.f, 0.f, 1.f)) * glm::angleAxis((float)cameraRotation.x, glm::vec3(0, 1, 0)));

            // Scales and rotates the world. For testing purposes.
            // scene.worlds[0].transform.setLocalRotation(glm::angleAxis((float)totalElapsedTime, glm::normalize(glm::vec3(-1.f, 0, 0))));
            // scene.worlds[0].transform.setLocalScale(glm::vec3(1, 1, 2));

            renderer.prepareRayTraceFromCamera(camera);
            renderer.executeRayTrace(scene.worlds);
            renderer.display();
        }

        {
            std::string str = "Hello";
            float f;
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

                    // if (ImGui::Button("Save"))
                    //     ;
                    // ImGui::InputText("string", &str);
                    // ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
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

void Program::log(const std::string& value)
{
    std::cout << value + "\n"
              << std::flush;
}

void Program::checkForContentFolder()
{
    if (!std::filesystem::is_directory("content"))
    {
        throw std::runtime_error("Could not find content folder. Is the working directory set correctly?");
    }

    log("Found content folder");
}

void Program::assertIsTrue(const bool condition, const std::string& errorMessage)
{
    if (!condition)
    {
        throw std::runtime_error("Assert failed: " + errorMessage);
    }
}

void Program::runEarlyStartupTests()
{
    log("Running early startup tests (in constructor)");

    {
        // Make sure the content folder exists
        checkForContentFolder();
    }

    {
        // This allows us to safely use nullptr instead of NULL
        assertIsTrue(NULL == nullptr, "Unsupported compiler: NULL must equal nullptr");
    }

    {
        // This allows us to safely use uint32_t instead of GLuint
        assertIsTrue(sizeof(GLuint) == sizeof(uint32_t), "Unsupported compiler: sizeof(GLuint) must equal sizeof(uint32_t)");
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

    {
        // Similar to before, but events are buffered when raised and only sent when sendBufferedEvents is called
        BufferedEvent<int> testEvent;
        int counter = 0;

        auto listener = testEvent.subscribe([&](int value)
            {
                log("Buffered event was successfully called");
                counter += value;
            });

        // Listener is subscribed, but event has not been flushed. This should not affect the counter
        testEvent.raise(5);
        assertIsTrue(counter == 0, "Incorrect buffered event implementation: counter should equal 0");

        // Flush events, this should add 5 to the counter
        testEvent.flush();
        assertIsTrue(counter == 5, "Incorrect buffered event implementation: counter should equal 5");

        // Unsubscribe from event
        listener.reset();

        // Listener is unsubscribed, this should not affect the counter
        testEvent.raise(5);
        testEvent.flush();
        assertIsTrue(counter == 5, "Incorrect buffered event implementation: counter should equal 5");
    }
}

void Program::runLateStartupTests()
{
    log("Running late startup tests (in run())");

    {
        // Verify shader storage block size is large enough
        GLint size;
        glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &size);
        log("GL_MAX_SHADER_STORAGE_BLOCK_SIZE is " + std::to_string(size) + " bytes.");

        // 134217728 is the GL_MAX_SHADER_STORAGE_BLOCK_SIZE of Exanite's laptop, also equal to 512x512x512
        assertIsTrue(size >= 134217728, "OpenGL driver not supported: GL_MAX_SHADER_STORAGE_BLOCK_SIZE is not big enough");
    }

    {
        // Verify shader storage block size is large enough
        glm::ivec3 workgroupSizes;
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &workgroupSizes.x);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &workgroupSizes.y);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &workgroupSizes.z);

        log("GL_MAX_COMPUTE_WORK_GROUP_SIZE is <" + std::to_string(workgroupSizes.x) + ", " + std::to_string(workgroupSizes.y) + ", " + std::to_string(workgroupSizes.z) + ">" + ".");

        glm::ivec3 workgroupCounts;
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &workgroupCounts.x);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &workgroupCounts.y);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &workgroupCounts.z);

        log("GL_MAX_COMPUTE_WORK_GROUP_COUNT is <" + std::to_string(workgroupCounts.x) + ", " + std::to_string(workgroupCounts.y) + ", " + std::to_string(workgroupCounts.z) + ">" + ".");
    }
}
