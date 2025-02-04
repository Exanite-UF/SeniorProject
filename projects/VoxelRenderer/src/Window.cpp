#include "Window.h"

Window::Window(const std::shared_ptr<InputManager>& inputManager)
{
    this->inputManager = inputManager;
}

GLFWmonitor* Window::getCurrentMonitor(GLFWwindow* window)
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

void Window::onWindowSize(GLFWwindow* window, int width, int height)
{
    // This is a static method, so we don't have access to our Window class
    // However, GLFWwindow contains a pointer to our Window class. This was set using glfwSetWindowUserPointer.
    // We now get the pointer and cast it as our Window pointer.
    auto self = static_cast<Window*>(glfwGetWindowUserPointer(window));

    // Only run the callbacks if there is a Window that the GLFWwindow belongs to.
    if (!self)
    {
        return;
    }

    glViewport(0, 0, width, height); // Adjusts the render target size for the window (ie. the render will resize to take up the full window)

    // TODO: call user specified callbacks
}

void Window::onKey(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // This is a static method, so we don't have access to our Window class
    // However, GLFWwindow contains a pointer to our Window class. This was set using glfwSetWindowUserPointer.
    // We now get the pointer and cast it as our Window pointer.
    auto self = static_cast<Window*>(glfwGetWindowUserPointer(window));

    // Only run the callbacks if there is a Window that the GLFWwindow belongs to.
    if (!self)
    {
        return;
    }

    // Call the inputManger callback for keyboard presses
    self->inputManager->onKey(window, key, scancode, action, mods);

    // TODO: call user specified callbacks
}

void Window::onMouseButton(GLFWwindow* window, int button, int action, int mods)
{
    // This is a static method, so we don't have access to our Window class
    // However, GLFWwindow contains a pointer to our Window class. This was set using glfwSetWindowUserPointer.
    // We now get the pointer and cast it as our Window pointer.
    auto self = static_cast<Window*>(glfwGetWindowUserPointer(window));

    // Only run the callbacks if there is a Window that the GLFWwindow belongs to.
    if (!self)
    {
        return;
    }

    // Call the inputManger callback for mouse clicks
    self->inputManager->onMouseButton(window, button, action, mods);

    // TODO: call user specified callbacks
}

void Window::onCursorPos(GLFWwindow* window, double xpos, double ypos)
{
    // This is a static method, so we don't have access to our Window class
    // However, GLFWwindow contains a pointer to our Window class. This was set using glfwSetWindowUserPointer.
    // We now get the pointer and cast it as our Window pointer.
    auto self = static_cast<Window*>(glfwGetWindowUserPointer(window));

    // Only run the callbacks if there is a Window that the GLFWwindow belongs to.
    if (!self)
    {
        return;
    }

    // Call the inputManger callback for mouse movement
    self->inputManager->onCursorPos(window, xpos, ypos);

    // TODO: call user specified callbacks
}

void Window::onScroll(GLFWwindow* window, double xoffset, double yoffset)
{
    // This is a static method, so we don't have access to our Window class
    // However, GLFWwindow contains a pointer to our Window class. This was set using glfwSetWindowUserPointer.
    // We now get the pointer and cast it as our Window pointer.
    auto self = static_cast<Window*>(glfwGetWindowUserPointer(window));

    // Only run the callbacks if there is a Window that the GLFWwindow belongs to.
    if (!self)
    {
        return;
    }

    // Call the inputManger callback for mouse scrolling
    self->inputManager->onScroll(window, xoffset, yoffset);
}

void Window::onCursorEnter(GLFWwindow* window, int entered)
{
    // This is a static method, so we don't have access to our Window class
    // However, GLFWwindow contains a pointer to our Window class. This was set using glfwSetWindowUserPointer.
    // We now get the pointer and cast it as our Window pointer.
    auto self = static_cast<Window*>(glfwGetWindowUserPointer(window));

    // Only run the callbacks if there is a Window that the GLFWwindow belongs to.
    if (!self)
    {
        return;
    }

    self->inputManager->invalidateMouse = true;
}

void Window::registerCallbacks()
{
    glfwSetWindowUserPointer(glfwWindowHandle, this); // This binds our Window wrapper class to the GLFWwindow object.

    // This binds the various callback functions
    glfwSetWindowSizeCallback(glfwWindowHandle, &Window::onWindowSize);
    glfwSetKeyCallback(glfwWindowHandle, &Window::onKey);
    glfwSetMouseButtonCallback(glfwWindowHandle, &Window::onMouseButton);
    glfwSetCursorPosCallback(glfwWindowHandle, &Window::onCursorPos);
    glfwSetScrollCallback(glfwWindowHandle, &Window::onScroll);
    glfwSetCursorEnterCallback(glfwWindowHandle, &Window::onCursorEnter);
}

void Window::setFullscreen()
{
    // Saving last windowed information
    glfwGetWindowPos(glfwWindowHandle, &lastWindowX, &lastWindowY);
    glfwGetWindowSize(glfwWindowHandle, &lastWindowWidth, &lastWindowWidth);

    GLFWmonitor* currentMonitor = getCurrentMonitor(glfwWindowHandle);
    const GLFWvidmode* mode = glfwGetVideoMode(currentMonitor);
    glfwSetWindowMonitor(glfwWindowHandle, currentMonitor, 0, 0, mode->width, mode->height, mode->refreshRate);
}

void Window::setWindowed()
{
    glfwSetWindowMonitor(glfwWindowHandle, nullptr, lastWindowX, lastWindowY, lastWindowWidth, lastWindowHeight, 0);
}
