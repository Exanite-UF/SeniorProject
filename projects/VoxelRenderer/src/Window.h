#pragma once

#include "InputManager.h"
#include <GLFW/glfw3.h>
#include <glm/common.hpp>
#include <memory>
#include <unordered_map>

// TODO: Not all of these need to be part of the Window
extern std::unordered_map<std::string, GLuint> shaderPrograms;
extern bool invalidateMouse;

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

    void update()
    {
        glfwPollEvents();
        inputManager->update();
    }

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
            self->inputManager->onKey(window, key, scancode, action, mods);
        }
    };

    static void onMouseButton(GLFWwindow* window, int button, int action, int mods)
    {
        auto self = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
        if (self)
        {
            self->inputManager->onMouseButton(window, button, action, mods);
        }
    };

    static void onCursorPos(GLFWwindow* window, double xpos, double ypos)
    {
        auto self = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
        if (self)
        {
            self->inputManager->onCursorPos(window, xpos, ypos);
        }
    };

    static void onScroll(GLFWwindow* window, double xoffset, double yoffset)
    {
        auto self = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
        if (self)
        {
            self->inputManager->onScroll(window, xoffset, yoffset);
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
