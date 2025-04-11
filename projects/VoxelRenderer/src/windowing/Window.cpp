#include <stdexcept>

#include <src/windowing/Window.h>

Window::Window(const std::string& contextName, GlfwContext* shareWith, bool enableImGui, bool isMainWindow)
    : GlfwContext(contextName, true, shareWith)
{
    this->enableImGui = enableImGui;
    this->isMainWindow = isMainWindow;

    // Register GLFW callbacks
    registerGlfwCallbacks();

    // Initialize state
    glfwGetWindowPos(glfwWindowHandle, &lastWindowedPosition.x, &lastWindowedPosition.y);
    glfwGetWindowSize(glfwWindowHandle, &lastWindowedSize.x, &lastWindowedSize.y);

    // Use raw mouse motion if supported
    // Raw mouse motion disables features such as mouse acceleration
    if (glfwRawMouseMotionSupported())
    {
        glfwSetInputMode(glfwWindowHandle, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }

    // Enable vsync
    glfwSwapInterval(1);

    if (enableImGui)
    {
        // Init IMGUI
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls
        ImGui_ImplGlfw_InitForOpenGL(glfwWindowHandle, true);
        ImGui_ImplOpenGL3_Init();
    }

    // Add callbacks
    windowSizeEvent.subscribePermanently([](Window* window, int width, int height)
        {
            glViewport(0, 0, width, height); // Adjusts the render target size for the window (ie. the render will resize to take up the full window)
        });
}

Window::~Window()
{
    if (enableImGui)
    {
        // Shutdown IMGUI
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }
}

void Window::update()
{
    if (enableImGui)
    {
        // Update IMGUI
        ImGuiIO& io = ImGui::GetIO();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    if (isMainWindow)
    {
        // We can only poll events on the main thread
        glfwPollEvents();
    }

    glfwGetWindowSize(glfwWindowHandle, &size.x, &size.y);

    windowSizeEvent.flush();
    keyEvent.flush();
    mouseButtonEvent.flush();
    cursorPosEvent.flush();
    scrollEvent.flush();
    cursorEnterEvent.flush();
}

void Window::present()
{
    if (enableImGui)
    {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    glfwSwapBuffers(glfwWindowHandle);
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
