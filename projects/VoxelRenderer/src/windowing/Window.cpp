#include <stdexcept>

#include "Window.h"
#include <src/windowing/Window.h>

Window::Window()
{
    Window(NULL);
}

Window::Window(GLFWwindow* shareContextWith)
{
    // Configure GLFW and OpenGL
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4); // Request OpenGL 4.6
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // Use Core profile
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Block usage of deprecated APIs
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE); // Enable debug messages

    // Create the window
    glfwWindowHandle = glfwCreateWindow(1024, 1024, "Voxel Renderer", nullptr, shareContextWith);
    if (glfwWindowHandle == nullptr)
    {
        throw std::runtime_error("Failed to create window");
    }

    // Set the Window's OpenGL context to be used on the current thread
    glfwMakeContextCurrent(glfwWindowHandle);

    // Register GLFW callbacks
    registerGlfwCallbacks();

    // Initialize state
    glfwSwapInterval(0); // Disable vsync
    glfwGetWindowPos(glfwWindowHandle, &lastWindowedPosition.x, &lastWindowedPosition.y);
    glfwGetWindowSize(glfwWindowHandle, &lastWindowedSize.x, &lastWindowedSize.y);

    // Use raw mouse motion if supported
    // Raw mouse motion disables features such as mouse acceleration
    if (glfwRawMouseMotionSupported())
    {
        glfwSetInputMode(glfwWindowHandle, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }

    glfwSwapInterval(1); // Enable vsync
}

Window::~Window()
{
    glfwDestroyWindow(glfwWindowHandle);
}

void Window::update()
{
    glfwPollEvents();
    glfwGetWindowSize(glfwWindowHandle, &size.x, &size.y);

    windowSizeEvent.flush();
    keyEvent.flush();
    mouseButtonEvent.flush();
    cursorPosEvent.flush();
    scrollEvent.flush();
    cursorEnterEvent.flush();
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

    self->windowSizeEvent.raise(self, width, height);
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

    self->keyEvent.raise(self, key, scancode, action, mods);
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

    self->mouseButtonEvent.raise(self, button, action, mods);
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

    self->cursorPosEvent.raise(self, xpos, ypos);
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

    self->scrollEvent.raise(self, xoffset, yoffset);
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

    self->cursorEnterEvent.raise(self, entered);
}

void Window::registerGlfwCallbacks()
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
    glfwGetWindowPos(glfwWindowHandle, &lastWindowedPosition.x, &lastWindowedPosition.y);
    glfwGetWindowSize(glfwWindowHandle, &lastWindowedSize.x, &lastWindowedSize.y);

    GLFWmonitor* currentMonitor = getCurrentMonitor(glfwWindowHandle);
    const GLFWvidmode* mode = glfwGetVideoMode(currentMonitor);
    glfwSetWindowMonitor(glfwWindowHandle, currentMonitor, 0, 0, mode->width, mode->height, mode->refreshRate);
}

void Window::setWindowed()
{
    glfwSetWindowMonitor(glfwWindowHandle, nullptr, lastWindowedPosition.x, lastWindowedPosition.y, lastWindowedSize.x, lastWindowedSize.y, 0);
}
