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
    static void onOpenGlDebugMessage(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);

    static void checkForContentFolder();

    // Ran first thing when the class is constructed
    static void runEarlyStartupTests();

    // Ran first thing when the run() method is called
    // Use this if things like OpenGL need to have been initialized
    static void runLateStartupTests();

public:
    std::shared_ptr<Window> window;
    std::shared_ptr<InputManager> inputManager;

    GLFWwindow* offscreen_context;

    bool isWorkload = false; // View toggle
    bool isRand2 = true; // Noise type toggle
    float fillAmount = 0.6;
    bool remakeNoise = true;

    GLuint blitTextureGraphicsProgram;
    GLuint blitFramebufferGraphicsProgram;
    GLuint raymarcherGraphicsProgram;
    GLuint makeNoiseComputeProgram;
    GLuint makeMipMapComputeProgram;
    GLuint assignMaterialComputeProgram;

    Program();
    ~Program();

    void run();
};
