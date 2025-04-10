#pragma once

#include <src/gameobjects/GameObject.h>
#include <src/input/InputManager.h>
#include <src/utilities/NonCopyable.h>
#include <src/windowing/Window.h>
#include <src/world/SceneComponent.h>

class Program : public NonCopyable
{
private:
    static void checkForContentFolder();

    // Ran first thing when the class is constructed
    static void runEarlyStartupTests();

    // Ran first thing when the run() method is called
    // Use this if things like OpenGL need to have been initialized
    static void runLateStartupTests();

public:
    std::shared_ptr<GlfwContext> offscreenContext {};
    std::vector<std::shared_ptr<GlfwContext>> chunkModificationThreadContexts {};
    std::shared_ptr<Window> window {};

    std::shared_ptr<InputManager> inputManager {};

    std::shared_ptr<GameObject> sceneObject {};
    std::shared_ptr<SceneComponent> scene {};

    bool isWorkload = false; // View toggle
    bool useRandomNoise = true; // Noise type toggle
    float fillAmount = 0.6;
    bool isRemakeNoiseRequested = false;

    // Fps counter
    float fpsCycleTimer = 0;

    float currentDisplayFps = 0;
    float averageDisplayDeltaTime = 0;

    float currentRenderFps = 0;
    float averagedRenderDeltaTime = 0;

    float frameTimePerPixel = -1;

    Program();
    ~Program() override;

    void run();
};
