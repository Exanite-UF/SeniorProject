#pragma once

#include "InputManager.h"
#include <GLFW/glfw3.h>
#include <glm/common.hpp>
#include <memory>
#include <unordered_map>

// TODO: Not all of these need to be part of the Window
extern std::unordered_map<std::string, GLuint> shaderPrograms;//TODO: consider moving to a shader manager class. (The shader manager would be the only thing that interacts with the shader compiler)

//TODO: The intended behavior of this flag should be possible to recreate with only the information provided from the InputManager.
//Consider adding the required information to the InputManager. It needs to know when the mouse enters the window to recreate the behavior of this flag.
extern bool invalidateMouse; 

//TODO: Remove from Window.h. This is beyond the scope of what Window should be managing.
extern bool isWorkload; // View toggle
extern bool isRand2; // Noise type toggle
extern float fillAmount;
extern bool remakeNoise;

// These are only set when the switching between fullscreen and windowed
//TODO: Add these fields to the Window class
//TODO: Find a better name, since these specifically are used to record the state of the window prior to entering fullscreen. They do not always store what their name impiles.
extern int windowX;
extern int windowY;
extern int windowWidth;
extern int windowHeight;

//TODO: Remove from Window.h. This is beyond the scope of what Window should be managing.
extern double noiseTime;

class Window
{
public:
    GLFWwindow* glfwWindowHandle;//A pointer to the object that is the window
    std::shared_ptr<InputManager> inputManager = std::make_shared<InputManager>();//The input manager for this window

    void update()
    {
        glfwPollEvents();
        //The requisite changes to the state of inputManager have been made by the triggering of callbacks due to glfwPollEvents
        //This finalizes those changes
        inputManager->update();

        //TODO: call user callback functions
        //TODO: find a way to only trigger callbacks on the rising and falling edges of inputs. This will probably involve editing the onWindowSize ... etc functions to support calling user specified functions.
    }

    //Find the monitor that the window is most likely to be on
    //Based on window-monitor overlap
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

    //Gets called every time the window resizes
    static void onWindowSize(GLFWwindow* window, int width, int height)
    {
        //window, the parameter variable, stores a GLFWWindow pointer
        //We need to know which Window, the current class we are defining, the GLFWwindow belongs to
        //This finds that
        auto self = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));//Get the reciever of the callback (ie. which of our Window object was this call back for)
        //Only run the callbacks if there is a Window that that the GLFWwindow belongs to.

        if (self)
        {
            glViewport(0, 0, width, height);//Adjusts the render target size for the window (ie. the render will resize to take up the full window)
        }
    };

    static void onKey(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        //window, the parameter variable, stores a GLFWWindow pointer
        //We need to know which Window, the current class we are defining, the GLFWwindow belongs to
        //This finds that
        auto self = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
        //Only run the callbacks if there is a Window that that the GLFWwindow belongs to.

        if (self)
        {
            self->inputManager->onKey(window, key, scancode, action, mods);
        }
    };

    static void onMouseButton(GLFWwindow* window, int button, int action, int mods)
    {
        //window, the parameter variable, stores a GLFWWindow pointer
        //We need to know which Window, the current class we are defining, the GLFWwindow belongs to
        //This finds that
        auto self = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
        //Only run the callbacks if there is a Window that that the GLFWwindow belongs to.

        if (self)
        {
            self->inputManager->onMouseButton(window, button, action, mods);
        }
    };

    static void onCursorPos(GLFWwindow* window, double xpos, double ypos)
    {
        //window, the parameter variable, stores a GLFWWindow pointer
        //We need to know which Window, the current class we are defining, the GLFWwindow belongs to
        //This finds that
        auto self = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
        //Only run the callbacks if there is a Window that that the GLFWwindow belongs to.

        if (self)
        {
            self->inputManager->onCursorPos(window, xpos, ypos);
        }
    };

    static void onScroll(GLFWwindow* window, double xoffset, double yoffset)
    {
        //window, the parameter variable, stores a GLFWWindow pointer
        //We need to know which Window, the current class we are defining, the GLFWwindow belongs to
        //This finds that
        auto self = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
        //Only run the callbacks if there is a Window that that the GLFWwindow belongs to.

        if (self)
        {
            self->inputManager->onScroll(window, xoffset, yoffset);
        }
    };

    static void onCursorEnter(GLFWwindow* window, int entered)
    {
        //window, the parameter variable, stores a GLFWWindow pointer
        //We need to know which Window, the current class we are defining, the GLFWwindow belongs to
        //This finds that
        auto self = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
        //Only run the callbacks if there is a Window that that the GLFWwindow belongs to.

        if (self)
        {
            invalidateMouse = true;
        }
    };


    //This binds the callbacks so that glfwPollEvents will call them.
    void registerCallbacks()
    {
        glfwSetWindowUserPointer(glfwWindowHandle, this);//This binds our Window wrapper class to the GLFWwindow object.

        //This binds the various callback functions
        glfwSetWindowSizeCallback(glfwWindowHandle, &Window::onWindowSize);
        glfwSetKeyCallback(glfwWindowHandle, &Window::onKey);
        glfwSetMouseButtonCallback(glfwWindowHandle, &Window::onMouseButton);
        glfwSetCursorPosCallback(glfwWindowHandle, &Window::onCursorPos);
        glfwSetScrollCallback(glfwWindowHandle, &Window::onScroll);
        glfwSetCursorEnterCallback(glfwWindowHandle, &Window::onCursorEnter);
    }
};
