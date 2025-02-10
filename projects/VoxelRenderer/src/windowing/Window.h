#pragma once

#include <GLFW/glfw3.h>
#include <glm/common.hpp>
#include <memory>
#include <unordered_map>

#include <src/input/InputManager.h>

class Window
{
    // Stores last known window sizes/positions when not in fullscreen mode
    glm::i32vec2 lastWindowedPosition;
    glm::i32vec2 lastWindowedSize;

    void registerGlfwCallbacks();

public:
    std::shared_ptr<InputManager> inputManager;
    glm::i32vec2 size;

    GLFWwindow* glfwWindowHandle; // A pointer to the object that is the window

    // TODO: Currently Window depends on the input manager, this dependency should be reversed. This will be doable once events are properly implemented.
    explicit Window(const std::shared_ptr<InputManager>& inputManager);
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    ~Window();

    void update();

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

    void setFullscreen();

    void setWindowed();
};
