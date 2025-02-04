#pragma once

#include "InputManager.h"
#include <GLFW/glfw3.h>
#include <glm/common.hpp>
#include <memory>
#include <unordered_map>

class Window
{
public:
    GLFWwindow* glfwWindowHandle; // A pointer to the object that is the window
    std::shared_ptr<InputManager> inputManager = std::make_shared<InputManager>(); // The input manager for this window
    int lastWindowX;
    int lastWindowY;
    int lastWindowWidth;
    int lastWindowHeight;

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

    void setFullscreen();

    void setWindowed();
};
