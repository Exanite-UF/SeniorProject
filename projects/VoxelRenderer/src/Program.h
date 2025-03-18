#pragma once

#include <GL/glew.h>

#include <GLFW/glfw3.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <imgui/imgui_stdlib.h>

#include <string>

#include <src/input/InputManager.h>
#include <src/utilities/NonCopyable.h>
#include <src/windowing/Window.h>

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
    std::shared_ptr<GlfwContext> offscreenContext;
    std::shared_ptr<Window> window;

    std::shared_ptr<InputManager> inputManager;

    bool isWorkload = false; // View toggle
    bool useRandomNoise = true; // Noise type toggle
    float fillAmount = 0.6;
    bool isRemakeNoiseRequested = true;

    float currentFPS1 = 0;
    float averagedDeltaTime1 = 0;

    Program();
    ~Program() override;

    void run();
};
