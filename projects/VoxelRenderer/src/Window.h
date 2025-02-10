#pragma once

#include <GLFW/glfw3.h>
#include <glm/common.hpp>
#include <glm/vec2.hpp>
#include <memory>
#include <unordered_map>

#include "BufferedEvent.h"

class Window
{
    // Stores last known window sizes/positions when not in fullscreen mode
    glm::i32vec2 lastWindowedPosition = glm::i32vec2(0);
    glm::i32vec2 lastWindowedSize = glm::i32vec2(1);

    void registerGlfwCallbacks();

    // Gets called every time the window resizes
    static void onWindowSize(GLFWwindow* window, int width, int height);

    static void onKey(GLFWwindow* window, int key, int scancode, int action, int mods);

    static void onMouseButton(GLFWwindow* window, int button, int action, int mods);

    static void onCursorPos(GLFWwindow* window, double xpos, double ypos);

    static void onScroll(GLFWwindow* window, double xoffset, double yoffset);

    static void onCursorEnter(GLFWwindow* window, int entered);

public:
    glm::i32vec2 size = glm::i32vec2(1);

    GLFWwindow* glfwWindowHandle; // A pointer to the object that is the window

    // Buffered events need to be flushed. This is done in update().
    // When adding new events, make sure they are flushed.
    BufferedEvent<Window*, int, int> windowSizeEvent {};
    BufferedEvent<Window*, int, int, int, int> keyEvent {};
    BufferedEvent<Window*, int, int, int> mouseButtonEvent {};
    BufferedEvent<Window*, double, double> cursorPosEvent {};
    BufferedEvent<Window*, double, double> scrollEvent {};
    BufferedEvent<Window*, int> cursorEnterEvent {};

    Window();
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    ~Window();

    void update();

    // Find the monitor that the window is most likely to be on
    // Based on window-monitor overlap
    static GLFWmonitor* getCurrentMonitor(GLFWwindow* window);

    void setFullscreen();

    void setWindowed();
};
