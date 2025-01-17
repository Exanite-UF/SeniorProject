#include <iostream>
#include "GLFW/glfw3.h"

int main() {
    std::cout << "Hello world" << std::endl;

    if (!glfwInit())
    {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    auto window = glfwCreateWindow(800, 600, "Voxel Renderer", nullptr, nullptr);
    if (!window)
    {
        throw std::runtime_error("Failed to create window");
    }

    while (true)
    {

    }

    return 0;
}
