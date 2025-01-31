#pragma once

#include "InputManager.h"
#include <GLFW/glfw3.h>
#include <glm/common.hpp>
#include <memory>
#include <unordered_map>

// TODO: Not all of these need to be part of the Window
extern std::unordered_map<std::string, GLuint> shaderPrograms;
extern std::unordered_set<int> heldKeys;
extern std::array<double, 2> mousePos;
extern std::array<double, 2> pastMouse;
extern bool invalidateMouse;
extern double mouseWheel;

extern bool isWorkload; // View toggle
extern bool isRand2; // Noise type toggle
extern float fillAmount;
extern bool remakeNoise;

// These are only set when the switching between fullscreen and windowed
extern int windowX;
extern int windowY;
extern int windowWidth;
extern int windowHeight;
extern double noiseTime;

class Window
{
public:
    GLFWwindow* glfwWindowHandle;
    std::shared_ptr<InputManager> inputManager = std::make_shared<InputManager>();

    static GLFWmonitor* getCurrentMonitor(GLFWwindow* window)
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

            overlap = glm::max(0, glm::min(wx + ww, mx + mw) - glm::max(wx, mx)) * glm::max(0, glm::min(wy + wh, my + mh) - glm::max(wy, my));

            if (bestoverlap < overlap)
            {
                bestoverlap = overlap;
                bestmonitor = monitors[i];
            }
        }

        return bestmonitor;
    }

    static void onWindowSize(GLFWwindow* window, int width, int height)
    {
        auto self = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
        if (self)
        {
            glViewport(0, 0, width, height);
        }
    };

    static void onKey(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        auto self = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
        if (self)
        {
            if (action == GLFW_PRESS && key == GLFW_KEY_F)
            {
                GLFWmonitor* monitor = glfwGetWindowMonitor(window);
                if (monitor == NULL)
                {
                    GLFWmonitor* currentMonitor = getCurrentMonitor(window);
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
            mousePos[0] = xpos;
            mousePos[1] = ypos;
        }
    };

    static void onScroll(GLFWwindow* window, double xoffset, double yoffset)
    {
        auto self = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
        if (self)
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
    };

    static void onCursorEnter(GLFWwindow* window, int entered)
    {
        auto self = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
        if (self)
        {
            invalidateMouse = true;
        }
    };

    void registerCallbacks()
    {
        glfwSetWindowUserPointer(glfwWindowHandle, this);

        glfwSetWindowSizeCallback(glfwWindowHandle, &Window::onWindowSize);
        glfwSetKeyCallback(glfwWindowHandle, &Window::onKey);
        glfwSetMouseButtonCallback(glfwWindowHandle, &Window::onMouseButton);
        glfwSetCursorPosCallback(glfwWindowHandle, &Window::onCursorPos);
        glfwSetScrollCallback(glfwWindowHandle, &Window::onScroll);
        glfwSetCursorEnterCallback(glfwWindowHandle, &Window::onCursorEnter);
    }
};
