#pragma once

#include "InputManager.h"
#include <GLFW/glfw3.h>
#include <memory>
#include <unordered_map>

// TODO: Not all of these need to be part of the Window
std::unordered_map<std::string, GLuint> shaderPrograms;
std::unordered_set<int> heldKeys;
std::array<double, 2> mousePos;
std::array<double, 2> pastMouse;
bool invalidateMouse = true;
double mouseWheel = 0;

bool isWorkload = false; // View toggle
bool isRand2 = true; // Noise type toggle
float fillAmount = 0.6;
bool remakeNoise = false;

// These are only set when the switching between fullscreen and windowed
int windowX = 0;
int windowY = 0;
int windowWidth = 0;
int windowHeight = 0;
double noiseTime = 0;

static int mini(int x, int y)
{
    return x < y ? x : y;
}

static int maxi(int x, int y)
{
    return x > y ? x : y;
}

GLFWmonitor* get_current_monitor(GLFWwindow* window)
{
    int nmonitors, i;
    int wx, wy, ww, wh;
    int mx, my, mw, mh;
    int overlap, bestoverlap;
    GLFWmonitor* bestmonitor;
    GLFWmonitor** monitors;
    const GLFWvidmode* mode;

    bestoverlap = 0;
    bestmonitor = NULL;

    glfwGetWindowPos(window, &wx, &wy);
    glfwGetWindowSize(window, &ww, &wh);
    monitors = glfwGetMonitors(&nmonitors);

    for (i = 0; i < nmonitors; i++)
    {
        mode = glfwGetVideoMode(monitors[i]);
        glfwGetMonitorPos(monitors[i], &mx, &my);
        mw = mode->width;
        mh = mode->height;

        overlap = maxi(0, mini(wx + ww, mx + mw) - maxi(wx, mx)) * maxi(0, mini(wy + wh, my + mh) - maxi(wy, my));

        if (bestoverlap < overlap)
        {
            bestoverlap = overlap;
            bestmonitor = monitors[i];
        }
    }

    return bestmonitor;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS && key == GLFW_KEY_F)
    {
        GLFWmonitor* monitor = glfwGetWindowMonitor(window);
        if (monitor == NULL)
        {
            GLFWmonitor* currentMonitor = get_current_monitor(window);
            glfwGetWindowPos(window, &windowX, &windowY);
            glfwGetWindowSize(window, &windowWidth, &windowHeight);

            const GLFWvidmode* mode = glfwGetVideoMode(currentMonitor);
            glfwSetWindowMonitor(window, currentMonitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        }
        else
        {
            glfwSetWindowMonitor(window, nullptr, windowX, windowY, windowWidth, windowHeight, 0);
        }
    }
    if (action == GLFW_PRESS && key == GLFW_KEY_Q)
    {
        int mode = glfwGetInputMode(window, GLFW_CURSOR);

        if (mode == GLFW_CURSOR_DISABLED)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
        else
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
    if (action == GLFW_PRESS && key == GLFW_KEY_ESCAPE)
    {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
    if (action == GLFW_PRESS && key == GLFW_KEY_R)
    {
        isWorkload = !isWorkload;
    }
    if (action == GLFW_PRESS && key == GLFW_KEY_T)
    {
        isRand2 = !isRand2;
        remakeNoise = true;
    }
    if (action == GLFW_PRESS)
    {
        heldKeys.insert(key);
    }
    if (action == GLFW_RELEASE)
    {
        heldKeys.erase(key);
    }
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    mousePos[0] = xpos;
    mousePos[1] = ypos;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (heldKeys.count(GLFW_KEY_LEFT_CONTROL))
    {
        fillAmount -= yoffset * 0.01;
        fillAmount = std::clamp(fillAmount, 0.f, 1.f);
        remakeNoise = true;
    }
    else
    {
        mouseWheel += yoffset;
    }
}

void window_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void cursor_enter_callback(GLFWwindow* window, int entered)
{
    if (entered)
    {
        // The cursor entered the content area of the window
        invalidateMouse = true;
    }
    else
    {
        // The cursor left the content area of the window
        invalidateMouse = true;
    }
}

class Window
{
public:
    GLFWwindow* glfwWindowHandle;
    std::shared_ptr<InputManager> inputManager;

    static void onWindowSize(GLFWwindow* window, int width, int height)
    {
        auto self = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
        if (self)
        {
            // Do stuff
        }
    };

    static void onKey(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        auto self = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
        if (self)
        {
            // Do stuff
        }
    };

    static void onMouseButton(GLFWwindow* window, int button, int action, int mods)
    {
        auto self = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
        if (self)
        {
            // Do stuff
        }
    };

    static void onCursorPos(GLFWwindow* window, double xpos, double ypos)
    {
        auto self = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
        if (self)
        {
            // Do stuff
        }
    };

    static void onScroll(GLFWwindow* window, double xoffset, double yoffset)
    {
        auto self = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
        if (self)
        {
            // Do stuff
        }
    };

    static void onCursorEnter(GLFWwindow* window, int entered)
    {
        auto self = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
        if (self)
        {
            // Do stuff
        }
    };

    void registerCallbacks()
    {
        glfwSetWindowUserPointer(glfwWindowHandle, this);

        glfwSetWindowSizeCallback(glfwWindowHandle, window_size_callback);
        glfwSetKeyCallback(glfwWindowHandle, key_callback);
        glfwSetCursorPosCallback(glfwWindowHandle, cursor_position_callback);
        glfwSetScrollCallback(glfwWindowHandle, scroll_callback);
        glfwSetCursorEnterCallback(glfwWindowHandle, cursor_enter_callback);

        // glfwSetWindowSizeCallback(glfwWindowHandle, &Window::onWindowSize);
        // glfwSetKeyCallback(glfwWindowHandle, &Window::onKey);
        // glfwSetMouseButtonCallback(glfwWindowHandle, &Window::onMouseButton);
        // glfwSetCursorPosCallback(glfwWindowHandle, &Window::onCursorPos);
        // glfwSetScrollCallback(glfwWindowHandle, &Window::onScroll);
        // glfwSetCursorEnterCallback(glfwWindowHandle, &Window::onCursorEnter);
    }
};
