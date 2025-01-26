// Include GLEW before OpenGL and GLFW
#include <GL/glew.h>

#include <GLFW/glfw3.h>

#include <glm/common.hpp>
#include <glm/vec4.hpp>
#include "glm/vec3.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

struct VertexPositionColor
{
    glm::vec3 position;
    glm::vec4 color;
};

void log(const std::string& value = "")
{
    std::cout << value + "\n" << std::flush;
}

void checkForContentFolder()
{
    if (!std::filesystem::is_directory("content"))
    {
        throw std::runtime_error("Could not find content folder. Is the working directory set correctly?");
    }
    else
    {
        log("Found content folder");
    }
}

GLuint createShaderModule(std::string path, GLenum type)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open file: " + path);
    }

    std::stringstream buffer{};
    buffer << file.rdbuf();

    std::string string = buffer.str();
    auto data = string.data();

    auto shader = glCreateShader(type);
    glShaderSource(shader, 1, &data, nullptr);
    glCompileShader(shader);

    GLint isSuccess;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &isSuccess);

    if (!isSuccess)
    {
        GLint messageLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &messageLength);

        std::string message{};
        message.resize(messageLength);

        glGetShaderInfoLog(shader, message.size(), nullptr, message.data());

        throw std::runtime_error("Failed to compile shader (" + path + "): " + message);
    }

    return shader;
}

GLuint createShaderProgram(std::string vertexShaderPath, std::string fragmentShaderPath)
{
    GLuint vertexModule = createShaderModule(vertexShaderPath, GL_VERTEX_SHADER);
    GLuint fragmentModule = createShaderModule(fragmentShaderPath, GL_FRAGMENT_SHADER);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexModule);
    glAttachShader(program, fragmentModule);
    {
        glLinkProgram(program);

        GLint isSuccess;
        glGetProgramiv(program, GL_LINK_STATUS, &isSuccess);

        if (!isSuccess) {
            GLint messageLength;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &messageLength);

            std::string message{};
            message.resize(messageLength);

            glGetProgramInfoLog(program, message.size(), nullptr, message.data());

            throw std::runtime_error("Failed to link shader program: " + message);
        }
    }
    glDetachShader(program, vertexModule);
    glDetachShader(program, fragmentModule);

    glDeleteShader(vertexModule);
    glDeleteShader(fragmentModule);

    return program;
}

int main()
{
    log("Starting Voxel Renderer");

    checkForContentFolder();

    // Init GLFW
    if (!glfwInit())
    {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    // Create window
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4); // Request OpenGL 4.6
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // Use Core profile
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Block usage of deprecated APIs
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

    // Create vertex input
    std::vector<VertexPositionColor> vertexData
    {
        VertexPositionColor{ glm::vec3(-0.5, -0.5, 0), glm::vec4(1, 0, 0, 1) },
        VertexPositionColor{ glm::vec3(0.5, -0.5, 0), glm::vec4(0, 1, 0, 1) },
        VertexPositionColor{ glm::vec3(0, 0.5, 0), glm::vec4(0, 0, 1, 1) },
    };

    // Vertex buffer
    GLuint vertexBuffer;
    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(VertexPositionColor), vertexData.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Vertex array
    GLuint vertexArray;
    glGenVertexArrays(1, &vertexArray);
    glBindVertexArray(vertexArray);

    // Read from vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);

    // Position (0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPositionColor), (void*)offsetof(VertexPositionColor, position));
    glEnableVertexAttribArray(0);

    // Color (1)
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(VertexPositionColor), (void*)offsetof(VertexPositionColor, color));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    GLuint emptyVertexArray;
    glGenVertexArrays(1, &emptyVertexArray);

    // Create shader program
    GLuint program = createShaderProgram("content/ScreenTri.vertex.glsl", "content/Raymarcher.fragment.glsl");

    // Main render loop
    glClearColor(0, 0, 0, 0);
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        {
            glUseProgram(program);
            glBindVertexArray(emptyVertexArray);

            glDrawArrays(GL_TRIANGLES, 0, 3);

            glUseProgram(0);
            glBindVertexArray(0);
        }
        glfwSwapBuffers(window);
    }

    return 0;
}
