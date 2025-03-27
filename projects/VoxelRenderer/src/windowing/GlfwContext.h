#pragma once

#include <atomic>

#include <src/utilities/NonCopyable.h>
#include <src/utilities/OpenGl.h>

class GlfwContext : public NonCopyable
{
private:
    static void onOpenGlDebugMessage(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);

protected:
    // Pointer returned by glfwCreateWindow
    GLFWwindow* glfwWindowHandle;

    inline static std::atomic<int> nextId;

    // Stores ID for logging and debugging purposes
    int id;

    explicit GlfwContext(bool isWindow, const GlfwContext* shareWith = nullptr);

public:
    explicit GlfwContext(GlfwContext* shareWith = nullptr);
    ~GlfwContext() override;

    void makeContextCurrent() const;

    GLFWwindow* getGlfwWindowHandle() const;
};
