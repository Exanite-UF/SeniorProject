// Include GLEW before OpenGL and GLFW
#include <GL/glew.h>

#include <iostream>
#include <GLFW/glfw3.h>
#include <glm/common.hpp>
#include <glm/vec4.hpp>

int main() {
    std::cout << "Starting Voxel Renderer" << std::endl;

    // Init GLFW
    if (!glfwInit())
    {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    // Create window
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    auto window = glfwCreateWindow(800, 600, "Voxel Renderer", nullptr, nullptr);
    if (window == nullptr)
    {
        throw std::runtime_error("Failed to create window");
    }

    // Init GLEW
    glfwMakeContextCurrent(window);
    glewExperimental = true;
    if (glewInit() != GLEW_OK)
    {
        throw std::runtime_error("Failed to initialize GLEW");
    }

    // Main render loop
    glm::vec4 clearColor{1, 0, 0, 1}; // TODO: Probably remove this vec4 usage, this is just to test GLM
    glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        glClear(GL_COLOR_BUFFER_BIT);

        glfwSwapBuffers(window);
    }

    return 0;
}
