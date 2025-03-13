#include <src/utilities/Log.h>
#include <stdexcept>
#include <string>

#include "GlfwContext.h"

int GlfwContext::nextId = 0;

GlfwContext::GlfwContext(bool isWindow, GlfwContext* shareWith)
{
    // Set ID for debugging
    id = nextId;
    nextId++;

    // Configure GLFW and OpenGL
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4); // Request OpenGL 4.6
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // Use Core profile
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Block usage of deprecated APIs
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE); // Enable debug messages
    glfwWindowHint(GLFW_VISIBLE, isWindow ? GLFW_TRUE : GLFW_FALSE); // Show window only if the context is to be used for a window

    // Create window/context
    auto shareWithHandle = shareWith != nullptr ? shareWith->glfwWindowHandle : nullptr;
    glfwWindowHandle = glfwCreateWindow(1024, 1024, "", nullptr, shareWithHandle);
    if (glfwWindowHandle == nullptr)
    {
        throw std::runtime_error("Failed to create GLFW window");
    }

    // Set the Window's OpenGL context to be used on the current thread
    glfwMakeContextCurrent(glfwWindowHandle);

    // Init GLEW
    glewExperimental = true;
    if (glewInit() != GLEW_OK)
    {
        throw std::runtime_error("Failed to initialize GLEW");
    }

    // Attach debug message callback
    glDebugMessageCallback(onOpenGlDebugMessage, this);
}

GlfwContext::GlfwContext(GlfwContext* shareWith)
    : GlfwContext(false, shareWith)
{
}

GlfwContext::~GlfwContext()
{
    glfwDestroyWindow(glfwWindowHandle);
}

void GlfwContext::makeContextCurrent() const
{
    // Set the Window's OpenGL context to be used on the current thread
    glfwMakeContextCurrent(glfwWindowHandle);
}

void GlfwContext::onOpenGlDebugMessage(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
    {
        return;
    }

    auto self = static_cast<const GlfwContext*>(userParam);
    if (self == nullptr)
    {
        return;
    }

    std::string messageStr(message, length);
    Log::log("GL context " + std::to_string(self->id) + ": " + messageStr);
}

GLFWwindow* GlfwContext::getGlfwWindowHandle() const
{
    return glfwWindowHandle;
}
