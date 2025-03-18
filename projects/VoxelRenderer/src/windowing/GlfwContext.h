#pragma once

#include <GL/glew.h>

#include <GLFW/glfw3.h>

#include <src/utilities/NonCopyable.h>

class GlfwContext : public NonCopyable
{
private:
    static void onOpenGlDebugMessage(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);

protected:
    // Pointer returned by glfwCreateWindow
    GLFWwindow* glfwWindowHandle;

    // Not thread safe
    static int nextId;

    // Stores ID for logging and debugging purposes
    int id;

    explicit GlfwContext(bool isWindow, const GlfwContext* shareWith = nullptr);

public:
    explicit GlfwContext(GlfwContext* shareWith = nullptr);
    ~GlfwContext() override;

    void makeContextCurrent() const;

    GLFWwindow* getGlfwWindowHandle() const;
};
