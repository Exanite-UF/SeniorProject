#include <src/utilities/ImGui.h>
#include <src/utilities/OpenGl.h>

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
#include <stb_image.h>

#include <CGAL/Distance_2/Point_2_Point_2.h>

#include <chrono>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

#include <src/Content.h>
#include <src/Program.h>
#include <src/gameobjects/GameObject.h>
#include <src/graphics/GraphicsUtility.h>
#include <src/graphics/ShaderManager.h>
#include <src/graphics/TextureManager.h>
#include <src/procgen/generators/ExampleWorldGenerator.h>
#include <src/procgen/generators/ExaniteWorldGenerator.h>
#include <src/procgen/generators/PrototypeWorldGenerator.h>
#include <src/procgen/generators/TextureHeightmapWorldGenerator.h>
#include <src/procgen/generators/WorldGenerator.h>
#include <src/procgen/synthesizers/TextureOctaveNoiseSynthesizer.h>
#include <src/procgen/synthesizers/TextureOpenSimplexNoiseSynthesizer.h>
#include <src/rendering/AsynchronousReprojection.h>
#include <src/rendering/Framebuffer.h>
#include <src/rendering/PostProcessing.h>
#include <src/rendering/Renderer.h>
#include <src/utilities/Assert.h>
#include <src/utilities/BufferedEvent.h>
#include <src/utilities/Event.h>
#include <src/utilities/Log.h>
#include <src/utilities/TupleHasher.h>
#include <src/windowing/Window.h>
#include <src/world/MaterialManager.h>
#include <src/world/SceneComponent.h>
#include <src/world/VoxelChunk.h>
#include <src/world/VoxelChunkData.h>
#include <src/world/VoxelChunkManager.h>
#include <src/world/VoxelChunkResources.h>

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

    offscreenContext = std::make_shared<GlfwContext>();
    window = std::make_shared<Window>(offscreenContext.get());

    window->makeContextCurrent();

    inputManager = std::make_shared<InputManager>(window);
}

Program::~Program()
{
    // Cleanup singletons
    SingletonManager::destroyAllSingletons();

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
    auto& voxelChunkResources = VoxelChunkResources::getInstance();
    auto& voxelChunkManager = VoxelChunkManager::getInstance();
    auto& input = inputManager->input;

    // Configure OpenGL
    glEnable(GL_FRAMEBUFFER_SRGB);
    glClearColor(0, 0, 0, 0);

    glEnable(GL_DEPTH_TEST);
    glClearDepth(0); // Reverse-Z
    glDepthFunc(GL_GREATER); // Reverse-Z
    // glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE); // Sets the Z clip range to [0, 1]

    // Create the scene GameObject
    auto sceneObject = GameObject::createRootObject("Scene");
    auto scene = sceneObject->addComponent<SceneComponent>();
    auto chunkSize = Constants::VoxelChunkComponent::chunkSize;

    // // Create the chunk GameObjects
    // for (int x = 0; x < 3; x++)
    // {
    //     for (int y = 0; y < 3; ++y)
    //     {
    //         auto voxelChunkObject = sceneObject->createChildObject("Chunk (" + std::to_string(x) + ", " + std::to_string(y) + ")");
    //
    //         auto voxelChunk = voxelChunkObject->addComponent<VoxelChunkComponent>(true);
    //         voxelChunk->getTransform()->addGlobalPosition(glm::vec3(chunkSize.x * x, chunkSize.y * y, 0) + glm::vec3(chunkSize.x / 2, chunkSize.y / 2, chunkSize.z / 2));
    //
    //         scene->addChunk(glm::ivec3(x, y, 0), voxelChunk);
    //     }
    // }

    // Create the camera GameObject
    auto cameraObject = sceneObject->createChildObject("Camera");
    auto camera = cameraObject->addComponent<CameraComponent>();
    auto& cameraTransform = camera->getTransform();
    scene->setCamera(camera);
    cameraTransform->setGlobalPosition(glm::vec3(0, 0, chunkSize.z * 1.25f));

    // Initialize the chunk manager
    voxelChunkManager.initialize(scene);

    // Create the renderer
    Renderer renderer(window, offscreenContext);
    float renderRatio = 0.5f; // Used to control the render resolution relative to the window resolution

    renderer.setRenderResolution(window->size); // Render resolution can be set separately from display resolution
    // renderer.setAsynchronousOverdrawFOV(10 * 3.1415926589 / 180);

    renderer.setRaysPerPixel(1);
    renderer.setBounces(2);

    // Configure post processing
    {
        // Gaussian blur
        if (false)
        {
            auto blurX = renderer.addPostProcessEffect(PostProcessEffect::getEffect("GaussianBlurX", ShaderManager::getInstance().getPostProcessProgram(Content::applyKernelLineFragmentShader)));
            blurX->setUniforms = [&renderer](GLuint program)
            {
                float standardDeviation = 2;
                int kernelRadius = standardDeviation * 2; // This will capture 96% of the expected input
                float lossCorrection = 1 / 0.954499736104; // Since some of the total is lost with a finite sized kernel, this multiplies the result by this correction factor

                std::vector<float> kernel;
                float sharedCoefficient = lossCorrection / std::sqrt(6.28318530718 * standardDeviation * standardDeviation);
                for (int i = -kernelRadius; i <= kernelRadius; i++)
                {
                    float dist = i * i;
                    kernel.push_back(sharedCoefficient * std::exp(-dist / (standardDeviation * standardDeviation) * 0.5));
                }

                glUniform1fv(glGetUniformLocation(program, "kernel"), 2 * kernelRadius + 1, kernel.data());

                glUniform1i(glGetUniformLocation(program, "kernelRadius"), kernelRadius); // This is the number of pixel away from the center (not including the center) that the kernel will apply to
                glUniform1i(glGetUniformLocation(program, "isXAxis"), true);
            };

            auto blurY = renderer.addPostProcessEffect(PostProcessEffect::getEffect("GaussianBlurY", ShaderManager::getInstance().getPostProcessProgram(Content::applyKernelLineFragmentShader)));
            blurY->setUniforms = [&renderer](GLuint program)
            {
                float standardDeviation = 2;
                int kernelRadius = standardDeviation * 2; // This will capture 96% of the expected input
                float lossCorrection = 1 / 0.954499736104; // Since some of the total is lost with a finite sized kernel, this multiplies the result by this correction factor

                std::vector<float> kernel;
                float sharedCoefficient = 1 / std::sqrt(6.28318530718 * standardDeviation * standardDeviation);
                for (int i = -kernelRadius; i <= kernelRadius; i++)
                {
                    float dist = i * i;
                    kernel.push_back(sharedCoefficient * std::exp(-dist / (standardDeviation * standardDeviation) * 0.5));
                }

                glUniform1fv(glGetUniformLocation(program, "kernel"), 2 * kernelRadius + 1, kernel.data());

                glUniform1i(glGetUniformLocation(program, "kernelRadius"), kernelRadius); // This is the number of pixel away from the center (not including the center) that the kernel will apply to
                glUniform1i(glGetUniformLocation(program, "isXAxis"), false);
            };
        }

        // Denoise approach 1
        if (false)
        {
            auto denoiseX = renderer.addPostProcessEffect(PostProcessEffect::getEffect("DenoiseX", ShaderManager::getInstance().getPostProcessProgram(Content::denoiseShader), GL_TEXTURE0, GL_TEXTURE1, GL_TEXTURE2, GL_TEXTURE3));
            denoiseX->setUniforms = [&renderer](GLuint program)
            {
                glUniform1i(glGetUniformLocation(program, "isXAxis"), true);
                glUniform3fv(glGetUniformLocation(program, "cameraPosition"), 1, glm::value_ptr(renderer.getCurrentCameraPosition()));
                glUniform4fv(glGetUniformLocation(program, "cameraRotation"), 1, glm::value_ptr(renderer.getCurrentCameraRotation()));
                glUniform1f(glGetUniformLocation(program, "cameraTanFOV"), std::tan(renderer.getCurrentCameraFOV() * 0.5));
                glUniform2iv(glGetUniformLocation(program, "resolution"), 1, glm::value_ptr(renderer.getUpscaleResolution()));
            };

            auto denoiseY = renderer.addPostProcessEffect(PostProcessEffect::getEffect("DenoiseY", ShaderManager::getInstance().getPostProcessProgram(Content::denoiseShader), GL_TEXTURE0, GL_TEXTURE1, GL_TEXTURE2, GL_TEXTURE3));
            denoiseY->setUniforms = [&renderer](GLuint program)
            {
                glUniform1i(glGetUniformLocation(program, "isXAxis"), false);
                glUniform3fv(glGetUniformLocation(program, "cameraPosition"), 1, glm::value_ptr(renderer.getCurrentCameraPosition()));
                glUniform4fv(glGetUniformLocation(program, "cameraRotation"), 1, glm::value_ptr(renderer.getCurrentCameraRotation()));
                glUniform1f(glGetUniformLocation(program, "cameraTanFOV"), std::tan(renderer.getCurrentCameraFOV() * 0.5));
                glUniform2iv(glGetUniformLocation(program, "resolution"), 1, glm::value_ptr(renderer.getUpscaleResolution()));
            };
        }

        // Denoise approach 2
        if (false)
        {
            auto denoise = renderer.addPostProcessEffect(PostProcessEffect::getEffect("Denoise", ShaderManager::getInstance().getPostProcessProgram(Content::denoise2Shader), GL_TEXTURE0, GL_TEXTURE1, GL_TEXTURE2, GL_TEXTURE3));
            denoise->setUniforms = [&renderer](GLuint program)
            {
                glUniform3fv(glGetUniformLocation(program, "cameraPosition"), 1, glm::value_ptr(renderer.getCurrentCameraPosition()));
                glUniform4fv(glGetUniformLocation(program, "cameraRotation"), 1, glm::value_ptr(renderer.getCurrentCameraRotation()));
                glUniform1f(glGetUniformLocation(program, "cameraTanFOV"), std::tan(renderer.getCurrentCameraFOV() * 0.5));
                glUniform2iv(glGetUniformLocation(program, "resolution"), 1, glm::value_ptr(renderer.getUpscaleResolution()));
            };
        }
    }

    // Engine time
    double totalElapsedTime = 0;
    auto previousTimestamp = std::chrono::high_resolution_clock::now();

    // Temporal accumulation
    int frameCount = 0;
    int maxFrames = 0; // TODO: Currently unused?

    bool shouldRenderPathTrace = true; // TODO: Currently unused?

    // Procedural Generation
    ExampleWorldGenerator exampleWorldGenerator {};
    ExaniteWorldGenerator exaniteWorldGenerator {};

    int seed = 0;
    int octaves = 3;
    float persistence = 0.5;
    auto octaveSynthesizer = std::make_shared<TextureOctaveNoiseSynthesizer>(seed, octaves, persistence);
    auto openSimplexSynthesizer = std::make_shared<TextureOpenSimplexNoiseSynthesizer>(seed);

    TextureHeightmapWorldGenerator octaveWorldGenerator(openSimplexSynthesizer);
    octaveWorldGenerator.setChunkSize(chunkSize);

    PrototypeWorldGenerator prototypeWorldGenerator(octaveSynthesizer);
    prototypeWorldGenerator.setChunkSize(chunkSize);

    // IMGUI Menu
    bool showMenuGUI = false;

    renderer.setScene(scene);
    renderer.startAsynchronousReprojection();

    while (!glfwWindowShouldClose(window->getGlfwWindowHandle()))
    {
        // Engine time
        auto currentTimestamp = std::chrono::high_resolution_clock::now();
        double deltaTime = std::chrono::duration<double>(currentTimestamp - previousTimestamp).count();
        previousTimestamp = currentTimestamp;
        totalElapsedTime += deltaTime;

        // Fps counter
        fpsCycleTimer += deltaTime;
        if (fpsCycleTimer > 1)
        {
            int displaysThisCycle = renderer.getReprojectionCounter();
            renderer.resetReprojectionCounter();
            currentDisplayFps = displaysThisCycle / fpsCycleTimer;
            averageDisplayDeltaTime = fpsCycleTimer / displaysThisCycle;

            int rendersThisCycle = renderer.getRenderCounter();
            renderer.resetRenderCounter();
            currentRenderFps = rendersThisCycle / fpsCycleTimer;
            averagedRenderDeltaTime = fpsCycleTimer / rendersThisCycle;

            auto averagedDisplayDeltaTimeMs = averageDisplayDeltaTime * 1000;
            auto averagedRenderDeltaTimeMs = averagedRenderDeltaTime * 1000;
            Log::log(std::format("{} display FPS ({} ms) | {} render FPS ({} ms)", currentDisplayFps, averagedDisplayDeltaTimeMs, currentRenderFps, averagedRenderDeltaTimeMs));

            fpsCycleTimer = 0;
        }

        // Update systems
        window->update();
        inputManager->update();
        voxelChunkManager.update(deltaTime);
        sceneObject->update();

        // Update
        // TODO: This code should be moved into individual systems
        {
            if (!inputManager->cursorEnteredThisFrame)
            {
                auto mouseDelta = input->getMouseDelta();

                camera->rotation.y -= mouseDelta.x * camera->mouseSensitivity;
                camera->rotation.x += mouseDelta.y * camera->mouseSensitivity;
                camera->rotation.x = glm::clamp(camera->rotation.x, -glm::pi<float>() / 2, glm::pi<float>() / 2);

                cameraTransform->setGlobalRotation(glm::angleAxis(camera->rotation.y, glm::vec3(0.f, 0.f, 1.f)) * glm::angleAxis(camera->rotation.x, glm::vec3(0, 1, 0)));
            }
            else
            {
                inputManager->cursorEnteredThisFrame = false;
            }

            auto cameraRightMoveDirection = camera->getRightMoveDirection();
            auto cameraForwardMoveDirection = camera->getForwardMoveDirection();
            auto cameraUpMoveDirection = camera->getUpMoveDirection();

            if (input->isKeyHeld(GLFW_KEY_A))
            {
                cameraTransform->addGlobalPosition(static_cast<float>(deltaTime * camera->moveSpeed) * -cameraRightMoveDirection);
            }

            if (input->isKeyHeld(GLFW_KEY_D))
            {
                cameraTransform->addGlobalPosition(static_cast<float>(deltaTime * camera->moveSpeed) * +cameraRightMoveDirection);
            }

            if (input->isKeyHeld(GLFW_KEY_W))
            {
                cameraTransform->addGlobalPosition(static_cast<float>(deltaTime * camera->moveSpeed) * +cameraForwardMoveDirection);
            }

            if (input->isKeyHeld(GLFW_KEY_S))
            {
                cameraTransform->addGlobalPosition(static_cast<float>(deltaTime * camera->moveSpeed) * -cameraForwardMoveDirection);
            }

            if (input->isKeyHeld(GLFW_KEY_SPACE))
            {
                cameraTransform->addGlobalPosition(static_cast<float>(deltaTime * camera->moveSpeed) * +cameraUpMoveDirection);
            }

            if (input->isKeyHeld(GLFW_KEY_LEFT_SHIFT))
            {
                cameraTransform->addGlobalPosition(static_cast<float>(deltaTime * camera->moveSpeed) * -cameraUpMoveDirection);
            }

            if (input->isKeyPressed(GLFW_KEY_G))
            {
                renderer.toggleAsynchronousReprojection();
            }

            std::shared_ptr<VoxelChunkComponent> closestChunk {};
            if (scene->tryGetClosestChunk(closestChunk))
            {
                if (input->isKeyHeld(GLFW_KEY_E) && closestChunk->getExistsOnGpu())
                {
                    closestChunk->getChunk()->generatePlaceholderData(deltaTime, useRandomNoise, fillAmount);
                }

                if (isRemakeNoiseRequested && closestChunk->getExistsOnGpu())
                {
                    // The noise time (corresponds to the deltaTime parameter) should not be incremented here
                    closestChunk->getChunk()->generatePlaceholderData(0, useRandomNoise, fillAmount);
                    isRemakeNoiseRequested = false;
                }

                if (input->isKeyPressed(GLFW_KEY_F6))
                {
                    exaniteWorldGenerator.generate(*closestChunk);
                }

                if (input->isKeyPressed(GLFW_KEY_F7))
                {
                    exampleWorldGenerator.generate(*closestChunk);
                }

                if (input->isKeyPressed(GLFW_KEY_F8))
                {
                    prototypeWorldGenerator.generate(*closestChunk);
                }
            }

            if (input->isKeyPressed(GLFW_KEY_F))
            {
                auto* monitor = glfwGetWindowMonitor(window->getGlfwWindowHandle());
                if (monitor == nullptr)
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
                int mode = glfwGetInputMode(window->getGlfwWindowHandle(), GLFW_CURSOR);

                if (mode == GLFW_CURSOR_DISABLED)
                {
                    glfwSetInputMode(window->getGlfwWindowHandle(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                }
                else
                {
                    glfwSetInputMode(window->getGlfwWindowHandle(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                }
            }
            if (input->isKeyPressed(GLFW_KEY_ESCAPE))
            {
                glfwSetWindowShouldClose(window->getGlfwWindowHandle(), GLFW_TRUE);
            }
            if (input->isKeyPressed(GLFW_KEY_R))
            {
                isWorkload = !isWorkload;
            }
            if (input->isKeyPressed(GLFW_KEY_T))
            {
                useRandomNoise = !useRandomNoise;
                isRemakeNoiseRequested = true;
            }

            // Scroll
            if (input->isKeyHeld(GLFW_KEY_LEFT_CONTROL) && input->getMouseScroll().y != 0)
            {
                fillAmount -= input->getMouseScroll().y * 0.01;
                fillAmount = std::clamp(fillAmount, 0.f, 1.f);
                isRemakeNoiseRequested = true;
            }
            else if (input->isKeyHeld(GLFW_KEY_LEFT_ALT) && input->getMouseScroll().y != 0)
            {
                renderRatio = glm::clamp(renderRatio + input->getMouseScroll().y * 0.01f, 0.01f, 2.0f);
            }
            else
            {
                camera->moveSpeed *= pow(1.1, input->getMouseScroll().y);
            }

            // F3 Debug Menu
            if (input->isKeyPressed(GLFW_KEY_F3))
            {
                showMenuGUI = !showMenuGUI;
            }

            //  Debug UI
            const int numMenus = 5;
            ImVec2 windowSize = ImVec2(window->size.x, window->size.y);
            float menuWidth = windowSize.x / numMenus;
            float menuHeight = windowSize.y / 4;
            const char* menuTitles[numMenus] = { "Stats (F3)", "Model Importer", "World Generation", "Controls", "About" };

            for (int i = 0; i < numMenus; i++)
            {
                ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.4f));
                ImGui::SetNextWindowPos(ImVec2(i * menuWidth, 0));
                ImGui::SetNextWindowSize(ImVec2(menuWidth, menuHeight));
                ImGui::SetNextWindowCollapsed(true, ImGuiCond_FirstUseEver);

                if (i == 0 && showMenuGUI)
                {
                    ImGui::SetNextWindowCollapsed(false, ImGuiCond_Always);
                }
                else if (i == 0)
                {
                    ImGui::SetNextWindowCollapsed(true, ImGuiCond_Always);
                }

                ImGui::Begin(menuTitles[i], nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
                ImGui::PushTextWrapPos(ImGui::GetWindowContentRegionMax().x);
                switch (i)
                {
                    case 0:
                    {
                        auto cameraPosition = cameraTransform->getGlobalPosition();
                        auto cameraLookDirection = cameraTransform->getForwardDirection();

                        ImGui::Text("Camera Position");
                        ImGui::Text("\tX: %.2f Y: %.2f Z: %.2f", cameraPosition.x, cameraPosition.y, cameraPosition.z);

                        ImGui::Text("\n");

                        ImGui::Text("Camera Look Direction");
                        ImGui::Text("\tX: %.2f Y: %.2f Z: %.2f", cameraLookDirection.x, cameraLookDirection.y, cameraLookDirection.z);

                        ImGui::Text("\n");

                        ImGui::Text("FPS: %.2f (Display) | %.2f (Render)", currentDisplayFps, currentRenderFps);
                        ImGui::Text("Reprojection Enabled: %s", renderer.getIsAsynchronousReprojectionEnabled() ? "True" : "False");
                        ImGui::Text("Window Resolution: %d x %d", window->size.x, window->size.y);
                        ImGui::Text("Render Resolution: %d x %d", renderer.getRenderResolution().x, renderer.getRenderResolution().y);
                        ImGui::Text("Render Ratio: %.2f", renderRatio);

                        break;
                    }
                    case 1:
                    {
                        ImGui::Text("TO BE ADDED");

                        break;
                    }
                    case 2:
                    {
                        exaniteWorldGenerator.showDebugMenu();
                        exampleWorldGenerator.showDebugMenu();
                        octaveWorldGenerator.showDebugMenu();
                        prototypeWorldGenerator.showDebugMenu();

                        voxelChunkManager.showDebugMenu();

                        break;
                    }
                    case 3:
                    {
                        ImGui::Text("W - Forward");
                        ImGui::Text("S - Backward");
                        ImGui::Text("A - Left");
                        ImGui::Text("D - Right");
                        ImGui::Text("Esc - Close Application");
                        ImGui::Text("E - Iterate Noise Over Time");
                        ImGui::Text("F - Toggle Fullscreen");
                        ImGui::Text("Q - Toggle Mouse Input");
                        ImGui::Text("T - Change Noise Type");
                        ImGui::Text("G - Toggle Reprojection");
                        ImGui::Text("Mouse Scroll - Change Move Speed");
                        ImGui::Text("Ctrl + Mouse Scroll - Change Noise Fill");
                        ImGui::Text("Alt + Mouse Scroll - Change Render Resolution");

                        break;
                    }
                    case 4:
                    {
                        ImGui::Text("ABOUT");

                        break;
                    }
                }
                ImGui::PopTextWrapPos();
                ImGui::End();
                ImGui::PopStyleColor();
            }
        }

        // Render
        {
            renderer.setRenderResolution(glm::ivec2(window->size.x * renderRatio, window->size.y * renderRatio));

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glDepthFunc(GL_GREATER);

            renderer.pollCamera(camera);
            renderer.render();
            glFinish();

            frameCount++;
        }

        // Present
        window->present();
    }

    renderer.stopAsynchronousReprojection();
    sceneObject->destroy();
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
        // Verify GameObject destroy API
        auto root = GameObject::createRootObject("Root");

        auto child1 = root->createChildObject("Child1");
        auto child2 = root->createChildObject("Child2");
        Assert::isTrue(root->getTransform()->getChildren().size() == 2, "Root GameObject should have 2 children");

        child1->destroy();
        Assert::isTrue(root->getTransform()->getChildren().size() == 1, "Root GameObject should have 1 children");
        Assert::isTrue(!child1->getIsAlive(), "Child1 GameObject should have been destroyed");

        root->destroy();
        Assert::isTrue(!root->getIsAlive(), "Root GameObject should have been destroyed");
        Assert::isTrue(!child1->getIsAlive(), "Child1 GameObject should have been destroyed");
        Assert::isTrue(!child2->getIsAlive(), "Child2 GameObject should have been destroyed");
    }

    {
        // Verify shader storage block size is large enough
        GLint size;
        glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &size);
        Log::log("GL_MAX_SHADER_STORAGE_BLOCK_SIZE is " + std::to_string(size) + " bytes.");

        // 134217728 is the GL_MAX_SHADER_STORAGE_BLOCK_SIZE of Exanite's laptop, also equal to 512x512x512
        Assert::isTrue(size >= 134217728, "OpenGL driver not supported: GL_MAX_SHADER_STORAGE_BLOCK_SIZE is not big enough");
    }

    {
        // Log compute shader max work group sizes
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
        // Verify shader storage block size is large enough to store buffers needed by renderer
        GLint maxShaderBlockSize;
        glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &maxShaderBlockSize);

        Assert::isTrue(maxShaderBlockSize >= Constants::VoxelChunk::maxMaterialCount * sizeof(MaterialDefinition), "GL_MAX_SHADER_STORAGE_BLOCK_SIZE is not big enough to store all material definitions");
        Assert::isTrue(maxShaderBlockSize >= 2 * 256 * 256 * 512, "GL_MAX_SHADER_STORAGE_BLOCK_SIZE is not big enough to store material map for a voxel chunk of size 256x256x512, where each voxel takes 2 bytes");

        if (maxShaderBlockSize >= 2 * 512 * 512 * 512)
        {
            Log::log("512x512x512 sized voxel chunks are supported on this device!");
        }
        else
        {
            Log::log("512x512x512 sized voxel chunks are NOT supported on this device!");
        }
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

        Assert::isTrue(CGAL::make_uncertain(1).is_certain(), "Failed to call CGAL function");
    }
}
