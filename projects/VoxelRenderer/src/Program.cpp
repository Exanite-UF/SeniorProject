#include "Program.h"

#include <src/utilities/ImGui.h>
#include <src/utilities/OpenGl.h>

#include <tracy/Tracy.hpp>

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

#include <CGAL/Distance_2/Point_2_Point_2.h>

#include <chrono>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

#include <fstream>

#include <src/Content.h>
#include <src/Program.h>
#include <src/gameobjects/GameObject.h>
#include <src/graphics/GraphicsUtility.h>
#include <src/graphics/ShaderManager.h>
#include <src/graphics/TextureData.h>
#include <src/graphics/TextureManager.h>
#include <src/procgen/generators/ExampleWorldGenerator.h>
#include <src/procgen/generators/ExaniteWorldGenerator.h>
#include <src/procgen/generators/PointSynthesizerWorldGenerator.h>
#include <src/procgen/generators/PrototypeWorldGenerator.h>
#include <src/procgen/generators/TextureHeightmapWorldGenerator.h>
#include <src/procgen/generators/WorldGenerator.h>
#include <src/procgen/synthesizers/PoissonDiskPointSynthesizer.h>
#include <src/procgen/synthesizers/TextureOctaveNoiseSynthesizer.h>
#include <src/procgen/synthesizers/TextureOpenSimplexNoiseSynthesizer.h>
#include <src/rendering/AsynchronousReprojection.h>
#include <src/rendering/Framebuffer.h>
#include <src/rendering/PostProcessing.h>
#include <src/rendering/Renderer.h>
#include <src/utilities/Assert.h>
#include <src/utilities/BufferedEvent.h>
#include <src/utilities/Event.h>
#include <src/utilities/ImGuiUtility.h>
#include <src/utilities/Log.h>
#include <src/utilities/TupleHasher.h>
#include <src/voxelizer/ModelPreviewer.h>
#include <src/voxelizer/ModelVoxelizer.h>
#include <src/windowing/Window.h>
#include <src/world/MaterialManager.h>
#include <src/world/SceneComponent.h>
#include <src/world/SkyboxComponent.h>
#include <src/world/VoxelChunk.h>
#include <src/world/VoxelChunkData.h>
#include <src/world/VoxelChunkManager.h>
#include <src/world/VoxelChunkResources.h>

Program::Program()
{
    ZoneScoped;

    // Ensure preconditions are met
    runEarlyStartupTests();
    Log::information("Starting Voxel Renderer");

    // Init GLFW
    if (!glfwInit())
    {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    std::shared_ptr<GlfwContext> previousContext {};

    auto chunkModificationThreadCount = std::max(2u, std::thread::hardware_concurrency() / 2);
    for (int i = 0; i < chunkModificationThreadCount; ++i)
    {
        previousContext = std::make_shared<GlfwContext>(std::format("Chunk modification {}", i), previousContext.get());
        chunkModificationThreadContexts.push_back(previousContext);
    }

    previousContext = offscreenContext = std::make_shared<GlfwContext>("Offscreen context", previousContext.get());
    previousContext = window = std::make_shared<Window>("Main window", previousContext.get());

    window->makeContextCurrent();

    // load the recorded performace of the program
    std::ifstream performanceFile("performanceFile.txt");
    if (performanceFile.is_open())
    {
        performanceFile.read(reinterpret_cast<char*>(&frameTimePerPixel), sizeof(float));
    }

    inputManager = std::make_shared<InputManager>(window);
}

Program::~Program()
{
    ZoneScoped;

    {
        ZoneScopedN("Destroy scene");

        sceneObject->removeFromWorld();
    }

    {
        ZoneScopedN("Cleanup singletons");

        SingletonManager::destroyAllSingletons();
    }

    // save the recorded performace of the program
    std::ofstream performanceFile("performanceFile.txt", std::ios::binary);
    if (performanceFile.is_open())
    {
        performanceFile.write(reinterpret_cast<const char*>(&frameTimePerPixel), sizeof(float));
    }

    // Shutdown GLFW
    glfwTerminate();
}

void Program::run()
{
    try
    {
        // Ensure preconditions are met
        runLateStartupTests();

        auto& shaderManager = ShaderManager::getInstance();
        auto& textureManager = TextureManager::getInstance();
        auto& materialManager = MaterialManager::getInstance();
        auto& voxelChunkResources = VoxelChunkResources::getInstance();
        auto& voxelChunkManager = VoxelChunkManager::getInstance();
        auto& input = inputManager->input;

        TextureManager::getInstance().enableBindlessTextures();

        // Configure OpenGL
        glEnable(GL_FRAMEBUFFER_SRGB);
        glClearColor(0, 0, 0, 0);

        glEnable(GL_DEPTH_TEST);
        glClearDepth(0); // Reverse-Z
        glDepthFunc(GL_GREATER); // Reverse-Z
        // glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE); // Sets the Z clip range to [0, 1]

        // Create the scene GameObject
        sceneObject = GameObject::createRootObject("Scene");
        scene = sceneObject->addComponent<SceneComponent>();
        auto chunkSize = Constants::VoxelChunkComponent::chunkSize;

        auto skybox = sceneObject->addComponent<SkyboxComponent>("content/skybox2/skyboxIndirect.txt", "content/skybox2/skyboxSettings.txt");
        // auto skybox = sceneObject->addComponent<SkyboxComponent>("content/skybox/skyboxIndirect.txt", "content/skybox/skyboxSettings.txt");
        scene->setSkybox(skybox);

        // Generate static, noise-based placeholder chunks for testing purposes
        if (false)
        {
            voxelChunkManager.settings.isChunkLoadingEnabled = false;
            voxelChunkManager.settings.enableCulling = false;
            for (int x = 0; x < 2; x++)
            {
                for (int y = 0; y < 1; ++y)
                {
                    auto voxelChunkObject = sceneObject->createChildObject(std::format("Chunk ({}, {})", x, y));

                    auto voxelChunk = voxelChunkObject->addComponent<VoxelChunkComponent>(true);
                    voxelChunk->getTransform()->addGlobalPosition(glm::vec3(chunkSize.x * x, chunkSize.y * y, 0) + glm::vec3(chunkSize.x / 2, chunkSize.y / 2, chunkSize.z / 2));
                    // voxelChunk->getTransform()->setLocalScale(glm::vec3(1.0/8, 1.0/8, 1.0/8));
                    voxelChunk->getTransform()->setLocalScale(glm::vec3(x + 1, x + 1, x + 1));

                    scene->addWorldChunk(glm::ivec3(x, y, 0), voxelChunk);
                }
            }
        }

        // Create the camera GameObject
        auto cameraObject = sceneObject->createChildObject("Camera");
        auto camera = cameraObject->addComponent<CameraComponent>();
        camera->moveSpeed = 4.317 * 8; // Set the base speed to 4.317 meter a second (minecraft walking speed. This is 9.6 miles per hour, Steve is very fast)
        auto cameraTransform = camera->getTransform();
        scene->setCamera(camera);
        cameraTransform->setGlobalPosition(glm::vec3(0, 0, chunkSize.z * 0.5));

        // Initialize the chunk manager
        voxelChunkManager.initialize(scene, chunkModificationThreadContexts);

        // Create the renderer
        Renderer renderer(window, offscreenContext);

        float renderRatio = 0.66666666f;
        float targetReprojectionFPS = 20;
        // Render resolution can be set separately from display resolution
        // renderer.setAsynchronousOverdrawFOV(10 * 3.1415926589 / 180);

        int numberOfBounces = 3;
        renderer.setBounces(numberOfBounces);

        // Configure post processing
        {
            // Gaussian blur
            if (false)
            {
                auto blurX = renderer.addPostProcessEffect(PostProcessEffect::getEffect("GaussianBlurX", ShaderManager::getInstance().getPostProcessProgram(Content::applyKernelLineFragmentShader)->programId));
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

                auto blurY = renderer.addPostProcessEffect(PostProcessEffect::getEffect("GaussianBlurY", ShaderManager::getInstance().getPostProcessProgram(Content::applyKernelLineFragmentShader)->programId));
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
                auto denoiseX = renderer.addPostProcessEffect(PostProcessEffect::getEffect("DenoiseX", ShaderManager::getInstance().getPostProcessProgram(Content::denoiseShader)->programId, GL_TEXTURE0, GL_TEXTURE1, GL_TEXTURE2, GL_TEXTURE3));
                denoiseX->setUniforms = [&renderer](GLuint program)
                {
                    glUniform1i(glGetUniformLocation(program, "isXAxis"), true);
                    glUniform3fv(glGetUniformLocation(program, "cameraPosition"), 1, glm::value_ptr(renderer.getCurrentCameraPosition()));
                    glUniform4fv(glGetUniformLocation(program, "cameraRotation"), 1, glm::value_ptr(renderer.getCurrentCameraRotation()));
                    glUniform1f(glGetUniformLocation(program, "cameraTanFOV"), std::tan(renderer.getCurrentCameraFOV() * 0.5));
                    glUniform2iv(glGetUniformLocation(program, "resolution"), 1, glm::value_ptr(renderer.getUpscaleResolution()));
                };

                auto denoiseY = renderer.addPostProcessEffect(PostProcessEffect::getEffect("DenoiseY", ShaderManager::getInstance().getPostProcessProgram(Content::denoiseShader)->programId, GL_TEXTURE0, GL_TEXTURE1, GL_TEXTURE2, GL_TEXTURE3));
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
                auto denoise = renderer.addPostProcessEffect(PostProcessEffect::getEffect("Denoise", ShaderManager::getInstance().getPostProcessProgram(Content::denoise2Shader)->programId, GL_TEXTURE0, GL_TEXTURE1, GL_TEXTURE2, GL_TEXTURE3));
                denoise->setUniforms = [&renderer](GLuint program)
                {
                    glUniform3fv(glGetUniformLocation(program, "cameraPosition"), 1, glm::value_ptr(renderer.getCurrentCameraPosition()));
                    glUniform4fv(glGetUniformLocation(program, "cameraRotation"), 1, glm::value_ptr(renderer.getCurrentCameraRotation()));
                    glUniform1f(glGetUniformLocation(program, "cameraTanFOV"), std::tan(renderer.getCurrentCameraFOV() * 0.5));
                    glUniform2iv(glGetUniformLocation(program, "resolution"), 1, glm::value_ptr(renderer.getUpscaleResolution()));
                };
            }

            // Show other
            if (false)
            {
                auto showOther = renderer.addPostProcessEffect(PostProcessEffect::getEffect("ShowOther", ShaderManager::getInstance().getPostProcessProgram(Content::showOtherFragmentShader)->programId, GL_TEXTURE0, GL_TEXTURE1, GL_TEXTURE2, GL_TEXTURE3));
                showOther->setUniforms = [&renderer](GLuint program)
                {
                    glUniform1i(glGetUniformLocation(program, "whichTexture"), 1);
                };
            }

            // Tonemap
            if (false)
            {

                auto toneMap = renderer.addPostProcessEffect(PostProcessEffect::getEffect("ToneMap", ShaderManager::getInstance().getPostProcessProgram(Content::toneMapShader)->programId, GL_TEXTURE0, GL_TEXTURE0, GL_TEXTURE0, GL_TEXTURE0));
                toneMap->setUniforms = [&renderer](GLuint program) {

                };
            }

            // Show angular size
            if (false)
            {
                auto showAngularSize = renderer.addPostProcessEffect(PostProcessEffect::getEffect("ShowAngularSize", ShaderManager::getInstance().getPostProcessProgram(Content::showAngularSizeShader)->programId, GL_TEXTURE0, GL_TEXTURE0, GL_TEXTURE0, GL_TEXTURE0));
                showAngularSize->setUniforms = [&renderer](GLuint program)
                {
                    glUniform1f(glGetUniformLocation(program, "angularSize"), 0.5 * 3.1415926589 / 180);
                    glUniform1f(glGetUniformLocation(program, "horizontalFov"), renderer.getCurrentCameraFOV());
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
        auto poissonDiskPointSynthesizer = std::make_shared<PoissonDiskPointSynthesizer>(seed);

        TextureHeightmapWorldGenerator octaveWorldGenerator(openSimplexSynthesizer);
        octaveWorldGenerator.setChunkSize(chunkSize);

        PrototypeWorldGenerator prototypeWorldGenerator(octaveSynthesizer);
        prototypeWorldGenerator.setChunkSize(chunkSize);

        PointSynthesizerWorldGenerator pointSynthesizerWorldGenerator(poissonDiskPointSynthesizer);
        pointSynthesizerWorldGenerator.setChunkSize(chunkSize);

        // IMGUI Menu
        bool showMenuGUI = false;

        // Model Previewer
        std::shared_ptr<ModelPreviewer> modelPreviewer = std::make_shared<ModelPreviewer>();
        std::shared_ptr<ModelVoxelizer> modelVoxelizer = std::make_shared<ModelVoxelizer>();
        bool isModelLoaded = false;
        bool isModelVoxelized = false;

        renderer.setScene(scene);
        renderer.startAsynchronousReprojection();

        // Adjust render ratio to meet performance target
        window->windowSizeEvent.subscribePermanently([&renderRatio, this, targetReprojectionFPS](Window* window, int a, int b)
            {
                float pixels = window->size.x * window->size.y;
                if (frameTimePerPixel > 0)
                {
                    renderRatio = std::sqrt((1.0 / targetReprojectionFPS) / (frameTimePerPixel * pixels)); // Used to control the render resolution relative to the window resolution
                }

                if (renderRatio > 1)
                {
                    renderRatio = 1;
                }
            });

        while (!glfwWindowShouldClose(window->getGlfwWindowHandle()))
        {
            const char* frameId = "Main Loop";
            FrameMarkStart(frameId);

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

                {
                    auto temp = renderer.getRenderResolution();
                    float newSample = averagedRenderDeltaTime / (temp.x * temp.y);
                    if (frameTimePerPixel < 0)
                    {
                        frameTimePerPixel = newSample;
                    }
                    else
                    {
                        float weight = pow(0.9, rendersThisCycle);
                        frameTimePerPixel = weight * frameTimePerPixel + (1 - weight) * newSample;
                    }

                    // The performace is unusable, update the fps
                    if (currentRenderFps < 15)
                    {
                        float pixels = window->size.x * window->size.y;
                        if (frameTimePerPixel > 0)
                        {
                            renderRatio = std::sqrt((1.0 / targetReprojectionFPS) / (frameTimePerPixel * pixels)); // Used to control the render resolution relative to the window resolution
                        }

                        if (renderRatio > 1)
                        {
                            renderRatio = 1;
                        }
                    }
                }

                // This lets you find the resolution at which 30fps is possible
                // if(currentRenderFps - 30 > 10){
                //     renderRatio *= 1.05;
                // }else if(currentRenderFps - 30 < -5){
                //     renderRatio *= 0.95;
                // }

                auto averagedDisplayDeltaTimeMs = averageDisplayDeltaTime * 1000;
                auto averagedRenderDeltaTimeMs = averagedRenderDeltaTime * 1000;
                Log::information(std::format("{} display FPS ({} ms) | {} render FPS ({} ms)", currentDisplayFps, averagedDisplayDeltaTimeMs, currentRenderFps, averagedRenderDeltaTimeMs));

                fpsCycleTimer = 0;
            }

            // Update systems
            window->update();
            camera->resolution = window->size;
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
                    camera->rotation.x = glm::clamp(camera->rotation.x, glm::radians(-89.0f), glm::radians(89.0f));

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

                if (input->isKeyPressed(GLFW_KEY_J))
                {
                    isGroundMovementEnabled = !isGroundMovementEnabled;
                    groundCameraHeight = camera->getTransform()->getGlobalPosition().z;
                }

                if (true && isGroundMovementEnabled)
                {
                    auto result = scene->raycast(camera->getTransform()->getGlobalPosition(), glm::vec3(0.0, 0.0, -1));
                    // std::cout << result.first << " " << result.second.x << " " << result.second.y << " " << result.second.z << std::endl;
                    if (result.first > 0)
                    {
                        groundCameraHeight = result.second.z + 1.6 * 8;

                        glm::vec3 camPos = camera->getTransform()->getGlobalPosition();
                        float p = 1 - std::exp(-groundCameraSnapSpeed * deltaTime);
                        camPos.z = ((1 - p) * camPos.z + p * groundCameraHeight);

                        camera->getTransform()->setGlobalPosition(camPos);
                    }
                }

                if (input->isKeyPressed(GLFW_KEY_EQUAL))
                {
                    renderer.increaseFirstMipMapLevel();
                }

                if (input->isKeyPressed(GLFW_KEY_MINUS))
                {
                    renderer.decreaseFirstMipMapLevel();
                }

                if (input->isKeyPressed(GLFW_KEY_G))
                {
                    renderer.toggleAsynchronousReprojection();
                }

                if (input->isKeyPressed(GLFW_KEY_B))
                {
                    renderer.toggleRendering();
                }

                if (input->isKeyPressed(GLFW_KEY_V))
                {
                    if (numberOfBounces == 3)
                    {
                        numberOfBounces = 2;
                    }
                    else
                    {
                        numberOfBounces = 3;
                    }

                    renderer.setBounces(numberOfBounces);
                }

                std::shared_ptr<VoxelChunkComponent> closestChunk {};
                if (scene->tryGetClosestWorldChunk(closestChunk))
                {
                    if (input->isKeyHeld(GLFW_KEY_E) && closestChunk->getExistsOnGpu())
                    {
                        std::lock_guard lock(closestChunk->getMutex());

                        closestChunk->getChunk()->generatePlaceholderData(deltaTime, useIsosurfaceNoise, fillAmount);
                    }

                    if (isRemakeNoiseRequested && closestChunk->getExistsOnGpu())
                    {
                        std::lock_guard lock(closestChunk->getMutex());

                        // The noise time (corresponds to the deltaTime parameter) should not be incremented here
                        closestChunk->getChunk()->generatePlaceholderData(0, useIsosurfaceNoise, fillAmount);
                        isRemakeNoiseRequested = false;
                    }

                    if (input->isKeyPressed(GLFW_KEY_F6))
                    {
                        exaniteWorldGenerator.generate(closestChunk, scene);
                    }

                    if (input->isKeyPressed(GLFW_KEY_F7))
                    {
                        exampleWorldGenerator.generate(closestChunk, scene);
                    }

                    if (input->isKeyPressed(GLFW_KEY_F8))
                    {
                        prototypeWorldGenerator.generate(closestChunk, scene);
                    }

                    if (input->isKeyPressed(GLFW_KEY_F9))
                    {
                        pointSynthesizerWorldGenerator.generate(closestChunk, scene);
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
                if (input->isKeyPressed(GLFW_KEY_END))
                {
                    glfwSetWindowShouldClose(window->getGlfwWindowHandle(), GLFW_TRUE);
                }
                if (input->isKeyPressed(GLFW_KEY_R))
                {
                    isWorkload = !isWorkload;
                }
                if (input->isKeyPressed(GLFW_KEY_T))
                {
                    useIsosurfaceNoise = !useIsosurfaceNoise;
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

                    static bool showOriginalModelMenu = false;
                    static bool showVoxelizedModelMenu = false;

                    float indentSize = ImGui::GetWindowContentRegionMax().x / 16.0f;

                    static bool collapsedStates[6] = { true, true, true, true, true, true };

                    // Stats
                    if (i == 0 && input->isKeyPressed(GLFW_KEY_F3))
                    {
                        collapsedStates[i] = !collapsedStates[i];
                    }

                    ImGui::SetNextWindowCollapsed(collapsedStates[i], ImGuiCond_Always);

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
                            ImGui::Text("\tX: %.4f Y: %.4f Z: %.4f", cameraLookDirection.x, cameraLookDirection.y, cameraLookDirection.z);

                            ImGui::Text("\n");

                            ImGui::Text("FPS: %.2f (Display) | %.2f (Render)", currentDisplayFps, currentRenderFps);
                            ImGui::Text("Reprojection Enabled: %s", renderer.getIsAsynchronousReprojectionEnabled() ? "True" : "False");
                            ImGui::Text("Window Resolution: %d x %d", window->size.x, window->size.y);
                            ImGui::Text("Render Resolution: %d x %d", renderer.getRenderResolution().x, renderer.getRenderResolution().y);
                            ImGui::Text("Render Ratio: %.2f", renderRatio);
                            ImGui::Text("Number of bounces: %d", numberOfBounces);

                            break;
                        }
                        case 1:
                        {
                            // Should be relative path
                            // std::string modelFileName = "../../content/Triangulation/suzanne.obj";
                            std::string modelFileName = "content/Triangulation/suzanne.obj";

                            ImGui::Text("Please choose a file.");
                            ImGui::Indent(indentSize);
                            ImGui::PushID("ModelKey");
                            ImGui::InputText("", &modelFileName);
                            ImGui::PopID();
                            ImGui::Unindent(indentSize);

                            // Triangle Mesh Preview
                            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.750f, 0.625f, 0.5f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.129f, 0.460f, 0.405f, 0.5f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.0f, 0.0f, 0.5f));
                            if (ImGui::Button("Import & Preview Model"))
                            {
                                // static std::string lastFile = modelFileName;
                                showOriginalModelMenu = !showOriginalModelMenu;
                                if (showOriginalModelMenu)
                                {
                                    // make this set and load model so that the VAO/VBO work on same thread
                                    modelPreviewer->createTriangleWindow(window, modelVoxelizer, modelFileName);
                                    isModelLoaded = true;
                                }
                                else
                                {
                                    modelPreviewer->closeWindowTriangle();
                                    isModelLoaded = false;
                                }
                            }
                            ImGui::PopStyleColor(3);

                            // Voxel Mesh Preview
                            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.770f, 0.372f, 0.0f, 0.5f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.510f, 0.320f, 0.143f, 0.5f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.0f, 0.0f, 0.5f));

                            if (isModelLoaded)
                            {
                                if (ImGui::Button("Voxelize & Preview Model"))
                                {
                                    showVoxelizedModelMenu = !showVoxelizedModelMenu;

                                    if (showVoxelizedModelMenu)
                                    {
                                        modelPreviewer->createVoxelWindow(window, modelVoxelizer);
                                        isModelVoxelized = true;
                                    }
                                    else
                                    {
                                        modelPreviewer->closeWindowVoxel();
                                        isModelVoxelized = false;
                                    }
                                }
                            }
                            ImGui::PopStyleColor(3);

                            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.253f, 0.540f, 0.248f, 0.5f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.313f, 0.450f, 0.310f, 0.5f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.0f, 0.0f, 0.5f));

                            if (isModelVoxelized)
                            {
                                if (ImGui::Button("Add to world"))
                                {
                                }
                            }

                            ImGui::PopStyleColor(3);

                            break;
                        }
                        case 2:
                        {
                            exaniteWorldGenerator.showDebugMenu();
                            exampleWorldGenerator.showDebugMenu();
                            octaveWorldGenerator.showDebugMenu();
                            prototypeWorldGenerator.showDebugMenu();
                            pointSynthesizerWorldGenerator.showDebugMenu();

                            voxelChunkManager.showDebugMenu();

                            break;
                        }
                        case 3:
                        {
                            ImGui::Text("W - Forward");
                            ImGui::Text("S - Backward");
                            ImGui::Text("A - Left");
                            ImGui::Text("D - Right");
                            ImGui::Text("End - Close Application");
                            ImGui::Text("E - Iterate Noise Over Time");
                            ImGui::Text("F - Toggle Fullscreen");
                            ImGui::Text("Q - Toggle Mouse Input");
                            ImGui::Text("T - Change Noise Type");
                            ImGui::Text("G - Toggle Reprojection");
                            ImGui::Text("V - Toggle Bounce Count");
                            ImGui::Text("B - Pause/Unpause Rendering");
                            ImGui::Text("J - Toggle ground camera");
                            ImGui::Text("+/- Change viewable mip map level");
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
                    collapsedStates[i] = ImGui::IsWindowCollapsed();
                    ImGui::PopTextWrapPos();
                    ImGui::End();
                    ImGui::PopStyleColor();
                }
            }

            // Render
            {

                renderer.setRenderResolution(glm::ivec2(window->size.x * renderRatio, window->size.y * renderRatio));

                renderer.pollCamera(camera);
                renderer.render();
                glFinish();

                frameCount++;
            }

            // Present
            window->present();

            FrameMarkEnd(frameId);
            FrameMark;
        }

        renderer.stopAsynchronousReprojection();
    }
    catch (const std::runtime_error& e)
    {
        // TODO: No idea why this prints gibberish on Windows
        Log::fatal("Program crashed: Runtime error: " + std::string(e.what()));

        std::terminate();
    }
    catch (...)
    {
        Log::fatal("Program crashed: Unknown error");

        std::terminate();
    }
}

void Program::checkForContentFolder()
{
    ZoneScoped;

    if (!std::filesystem::is_directory("content"))
    {
        throw std::runtime_error("Could not find content folder. Is the working directory set correctly?");
    }

    Log::information("Found content folder");
}

void Program::runEarlyStartupTests()
{
    ZoneScoped;

    Log::information("Running early startup tests (in constructor)");

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
                Log::information("Event was successfully called");
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
                Log::information("Buffered event was successfully called");
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
    ZoneScoped;

    Log::information("Running late startup tests (in run())");

    {
        // Verify GameObject removeFromWorld API
        auto root = GameObject::createRootObject("Root");
        auto rootTransform = root->getTransform();

        auto child1 = root->createChildObject("Child1");
        auto child1Transform = child1->getTransform();

        auto child2 = root->createChildObject("Child2");
        auto child2Transform = child2->getTransform();

        Assert::isTrue(root->getTransform()->getChildren().size() == 2, "Root GameObject should have 2 children");

        child1->removeFromWorld();
        Assert::isTrue(root->getTransform()->getChildren().size() == 1, "Root GameObject should have 1 children");
        Assert::isTrue(!child1->getIsPartOfWorld(), "Child1 GameObject should have been removed from the world");

        root->removeFromWorld();
        Assert::isTrue(!root->getIsPartOfWorld(), "Root GameObject should have been removed from the world");
        Assert::isTrue(!child1->getIsPartOfWorld(), "Child1 GameObject should have been removed from the world");
        Assert::isTrue(!child2->getIsPartOfWorld(), "Child2 GameObject should have been removed from the world");
    }

    Assert::isTrue(GameObject::getInstanceCount() == 0, "No GameObjects should be currently alive");
    Assert::isTrue(Component::getInstanceCount() == 0, "No Components should be currently alive");

    {
        // Verify Transform setParent API
        auto root = GameObject::createRootObject("Root");

        auto child1 = root->createChildObject("Child1");
        auto child2 = root->createChildObject("Child2");
        Assert::isTrue(root->getTransform()->getChildren().size() == 2, "Root GameObject should have 2 children");

        child2->getTransform()->setParent(nullptr);
        Assert::isTrue(!child2->getTransform()->hasParent(), "Child2 GameObject should not have a parent");
        Assert::isTrue(root->getTransform()->getChildren().size() == 1, "Root GameObject should have 1 children");
        Assert::isTrue(root->getTransform()->getChildren().at(0)->getGameObject() == child1, "Root GameObject have Child1 as its only child");

        child2->removeFromWorld();
        root->removeFromWorld();
    }

    Assert::isTrue(GameObject::getInstanceCount() == 0, "No GameObjects should be currently alive");
    Assert::isTrue(Component::getInstanceCount() == 0, "No Components should be currently alive");

    {
        // Verify shader storage block size is large enough
        GLint size;
        glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &size);
        Log::information("GL_MAX_SHADER_STORAGE_BLOCK_SIZE is " + std::to_string(size) + " bytes.");

        // 134217728 is the GL_MAX_SHADER_STORAGE_BLOCK_SIZE of Exanite's laptop, also equal to 512x512x512
        Assert::isTrue(size >= 134217728, "OpenGL driver not supported: GL_MAX_SHADER_STORAGE_BLOCK_SIZE is not big enough");
    }

    {
        // Log compute shader max work group sizes
        glm::ivec3 workgroupSizes;
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &workgroupSizes.x);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &workgroupSizes.y);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &workgroupSizes.z);

        Log::information("GL_MAX_COMPUTE_WORK_GROUP_SIZE is <" + std::to_string(workgroupSizes.x) + ", " + std::to_string(workgroupSizes.y) + ", " + std::to_string(workgroupSizes.z) + ">" + ".");

        glm::ivec3 workgroupCounts;
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &workgroupCounts.x);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &workgroupCounts.y);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &workgroupCounts.z);

        Log::information("GL_MAX_COMPUTE_WORK_GROUP_COUNT is <" + std::to_string(workgroupCounts.x) + ", " + std::to_string(workgroupCounts.y) + ", " + std::to_string(workgroupCounts.z) + ">" + ".");
    }

    {
        // Verify shader storage block size is large enough to store buffers needed by renderer
        GLint maxShaderBlockSize;
        glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &maxShaderBlockSize);

        Assert::isTrue(maxShaderBlockSize >= Constants::VoxelChunk::maxMaterialCount * sizeof(MaterialDefinition), "GL_MAX_SHADER_STORAGE_BLOCK_SIZE is not big enough to store all material definitions");
        Assert::isTrue(maxShaderBlockSize >= 2 * 256 * 256 * 512, "GL_MAX_SHADER_STORAGE_BLOCK_SIZE is not big enough to store material map for a voxel chunk of size 256x256x512, where each voxel takes 2 bytes");

        if (maxShaderBlockSize >= 2 * 512 * 512 * 512)
        {
            Log::information("512x512x512 sized voxel chunks are supported on this device!");
        }
        else
        {
            Log::information("512x512x512 sized voxel chunks are NOT supported on this device!");
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

    {
        // Test LOD generation
        VoxelChunkData original(glm::ivec3(4, 4, 4));
        original.setVoxelOccupancy(glm::ivec3(1, 1, 1), true);
        original.setVoxelMaterialIndex(glm::ivec3(1, 1, 1), 7);

        VoxelChunkData lod {};

        original.copyToLod(lod);
        Assert::isTrue(lod.getVoxelOccupancy(glm::ivec3(0, 0, 0)), "Expected voxel to be occupied");
        Assert::isTrue(lod.getVoxelMaterialIndex(glm::ivec3(0, 0, 0)) == 7, "Expected voxel material index to be 7");
    }

    {
        // Test mipmap generation
        VoxelChunkData data(glm::ivec3(16, 16, 16));
        data.setVoxelOccupancy(glm::ivec3(3, 3, 3), true);
        data.setHasMipmaps(true);

        Assert::isTrue(data.getHasMipmaps(), "Expected getHasMipmaps() to be true");
        Assert::isTrue(data.getOccupancyLayerCount() == 2, "Expected 2 occupancy layers");
        Assert::isTrue(data.getOccupancyMipmapCount() == 1, "Expected 1 occupancy mipmap layers");
        Assert::isTrue(data.getMipmapVoxelOccupancy(glm::ivec3(3, 3, 3), 0), "Expected voxel to be occupied (level 0)");
        Assert::isTrue(data.getMipmapVoxelOccupancy(glm::ivec3(0, 0, 0), 1), "Expected voxel to be occupied (level 1)");
    }

    // runChunkHierarchyTest();
}

// Yes I know this include location is stupid
#include <src/procgen/ChunkHierarchyManager.h>
#include <src/procgen/WorldUtility.h>
#include <src/procgen/data/TreeStructure.h>

void Program::runChunkHierarchyTest()
{
    auto& manager = ChunkHierarchyManager::getInstance();
    manager.setChunkSize(glm::ivec3(512));

    auto& materialManager = MaterialManager::getInstance();

    std::shared_ptr<Material> oakLogMaterial;
    std::shared_ptr<Material> oakLeafMaterial;
    WorldUtility::tryGetMaterial("oak_log", materialManager, oakLogMaterial);
    WorldUtility::tryGetMaterial("oak_leaf", materialManager, oakLeafMaterial);

    glm::ivec2 leafSize = { 8, 8 }; // This is the number of unit that the leaves will span
    int treeHeight = 8 * 10;
    int trunkDiameter = 4;
    float leafFillPercent = 0.5;

    // Test a tree that fits in one chunk
    if (true)
    {
        {
            glm::ivec3 originVoxel = glm::ivec3(10, 10, 0);
            manager.addStructure(originVoxel, std::make_shared<TreeStructure>(TreeStructure(originVoxel, oakLogMaterial, oakLeafMaterial, treeHeight, trunkDiameter, leafSize.x, leafSize.y, 0, 10, leafFillPercent)));
        }

        {
            glm::ivec2 chunkPosition = { 0, 0 };
            auto temp = manager.getStructuresForChunk(chunkPosition);
            glm::ivec3 origin = (*temp.begin())->getOriginVoxel();
            Assert::isTrue(origin.x == 10 && origin.y == 10, "Expected Tree at 10, 10 to be accessible from chunk at (0, 0).");
        }

        manager.clear();
    }

    // Test a tree that crosses a chunk boundary
    if (true)
    {
        {
            glm::ivec3 originVoxel = glm::ivec3(0, 0, 0);
            manager.addStructure(originVoxel, std::make_shared<TreeStructure>(TreeStructure(originVoxel, oakLogMaterial, oakLeafMaterial, treeHeight, trunkDiameter, leafSize.x, leafSize.y, 0, 10, leafFillPercent)));
        }

        {
            glm::ivec2 chunkPosition = { -1, 0 };
            auto temp = manager.getStructuresForChunk(chunkPosition);
            glm::ivec3 origin = (*temp.begin())->getOriginVoxel();
            Assert::isTrue(origin.x == 0 && origin.y == 0, "Expected Tree at 0, 0 to be accessible from chunk at (-1, 0).");
        }

        manager.clear();
    }

    // Test a really large canopy tree
    if (true)
    {
        // A really large canopy tree
        // Should end up in at least (-3, -1) to (-1, 1)
        {
            glm::ivec3 originVoxel = glm::ivec3(-757, 245, 0);
            manager.addStructure(originVoxel, std::make_shared<TreeStructure>(TreeStructure(originVoxel, oakLogMaterial, oakLeafMaterial, treeHeight, trunkDiameter, 540 * 2, 540 * 2, 0, 10, leafFillPercent)));
        }

        for (int i = -4; i <= 1; i++)
        {
            for (int j = -2; j <= 2; j++)
            {
                glm::ivec2 chunkPosition = { i * 512, j * 512 };
                auto temp = manager.getStructuresForChunk(chunkPosition);

                if (i >= -3 && i <= -1 && j >= -1 && j <= 1)
                {
                    glm::ivec3 origin = (*temp.begin())->getOriginVoxel();
                    Assert::isTrue(origin.x == -757 && origin.y == 245, "Expected Tree at -757, 245 with radius 540 to be accessible in the rectangle (-1536, -512) to (-512, 512).");
                }
            }
        }

        manager.clear();
    }
}
