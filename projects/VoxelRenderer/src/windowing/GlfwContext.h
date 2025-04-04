#pragma once

#include <atomic>
#include <string>

#include <src/utilities/NonCopyable.h>
#include <src/utilities/OpenGl.h>

class GlfwContext : public NonCopyable
{
private:
    static void onOpenGlDebugMessage(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);

protected:
    // Pointer returned by glfwCreateWindow
    GLFWwindow* glfwWindowHandle;
    std::string contextName;

    explicit GlfwContext(const std::string& contextName, bool isWindow, const GlfwContext* shareWith = nullptr);

public:
    explicit GlfwContext(const std::string& contextName, const GlfwContext* shareWith = nullptr);
    ~GlfwContext() override;

    void makeContextCurrent() const;

    GLFWwindow* getGlfwWindowHandle() const;
};
