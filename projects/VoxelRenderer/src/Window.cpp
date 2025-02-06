#include "Window.h"

Window::Window(const std::shared_ptr<InputManager>& inputManager)
{
    this->inputManager = inputManager;

    // Configure GLFW and OpenGL
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4); // Request OpenGL 4.6
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // Use Core profile
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Block usage of deprecated APIs

    // Create the window
    glfwWindowHandle = glfwCreateWindow(1024, 1024, "Voxel Renderer", nullptr, nullptr);
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
}

Window::~Window()
{
    glfwDestroyWindow(glfwWindowHandle);
}

void Window::update()
{
    glfwPollEvents();
    // TODO: call user callback functions
    // TODO: find a way to only trigger callbacks on the rising and falling edges of inputs. This will probably involve editing the onWindowSize ... etc functions to support calling user specified functions.
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
