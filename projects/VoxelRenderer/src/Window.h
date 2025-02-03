#pragma once

#include "InputManager.h"
#include <GLFW/glfw3.h>
#include <glm/common.hpp>
#include <memory>
#include <unordered_map>

// TODO: Not all of these need to be part of the Window
extern std::unordered_map<std::string, GLuint> shaderPrograms; // TODO: consider moving to a shader manager class. (The shader manager would be the only thing that interacts with the shader compiler)

// TODO: Remove from Window.h. This is beyond the scope of what Window should be managing.
// The intended behavior of this flag should be possible to recreate with only the information provided from the InputManager.
// Consider adding the required information to the InputManager. It needs to know when the mouse enters the window to recreate the behavior of this flag.
extern bool invalidateMouse;

// TODO: Remove from Window.h. This is beyond the scope of what Window should be managing.
extern bool isWorkload; // View toggle
extern bool isRand2; // Noise type toggle
extern float fillAmount;
extern bool remakeNoise;

// These are only set when the switching between fullscreen and windowed
// TODO: Add these fields to the Window class
// TODO: Find a better name, since these specifically are used to record the state of the window prior to entering fullscreen. They do not always store what their name impiles.
extern int windowX;
extern int windowY;
extern int windowWidth;
extern int windowHeight;

// TODO: Remove from Window.h. This is beyond the scope of what Window should be managing.
extern double noiseTime;

class Window
{
public:
    GLFWwindow* glfwWindowHandle; // A pointer to the object that is the window
    std::shared_ptr<InputManager> inputManager = std::make_shared<InputManager>(); // The input manager for this window

    void update()
    {
        glfwPollEvents();
        // The requisite changes to the state of inputManager have been made by the triggering of callbacks due to glfwPollEvents
        // This finalizes those changes
        inputManager->update();

        // TODO: call user callback functions
        // TODO: find a way to only trigger callbacks on the rising and falling edges of inputs. This will probably involve editing the onWindowSize ... etc functions to support calling user specified functions.
    }

    // Find the monitor that the window is most likely to be on
    // Based on window-monitor overlap
    static GLFWmonitor* getCurrentMonitor(GLFWwindow* window);

    // Gets called every time the window resizes
    static void onWindowSize(GLFWwindow* window, int width, int height);

    static void onKey(GLFWwindow* window, int key, int scancode, int action, int mods);

    static void onMouseButton(GLFWwindow* window, int button, int action, int mods);

    static void onCursorPos(GLFWwindow* window, double xpos, double ypos);

    static void onScroll(GLFWwindow* window, double xoffset, double yoffset);

    static void onCursorEnter(GLFWwindow* window, int entered);

    // This binds the callbacks so that glfwPollEvents will call them.
    void registerCallbacks();

    void toFullscreen();

    void toWindowed();
};
