#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/common.hpp>
#include <glm/vec4.hpp>

int main() {
    std::cout << "Hello world" << std::endl;

    if (!glfwInit())
    {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    if (!glewInit())
    {
        throw std::runtime_error("Failed to initialize GLEW");
    }

    auto window = glfwCreateWindow(800, 600, "Voxel Renderer", nullptr, nullptr);
    if (!window)
    {
        throw std::runtime_error("Failed to create window");
    }

    glfwMakeContextCurrent(window);

    glm::vec4 clearColor{1, 0, 0, 1};

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
        glClear(GL_COLOR_BUFFER_BIT);

        glfwSwapBuffers(window);
    }

    return 0;
}
