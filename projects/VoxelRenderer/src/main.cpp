// Include GLEW before OpenGL and GLFW
#include <GL/glew.h>

#include <GLFW/glfw3.h>

#include <glm/common.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>
#include <unordered_map>
#include <unordered_set>

#include "InputManager.h"
#include "Window.h"

// WASD Space Shift = movement
// q = capture mouse
// f = fullscreen toggle
// e = progress through the noise's time
// t = change between types of noise
// scroll = change move speed
// CTRL + scroll = change noise fill

void log(const std::string& value = "")
{
    std::cout << value + "\n"
              << std::flush;
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

    std::stringstream buffer {};
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

        std::string message {};
        message.resize(messageLength);

        glGetShaderInfoLog(shader, message.size(), nullptr, message.data());

        throw std::runtime_error("Failed to compile shader (" + path + "): " + message);
    }

    return shader;
}

GLuint createGraphicsProgram(std::string vertexShaderPath, std::string fragmentShaderPath)
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

        if (!isSuccess)
        {
            GLint messageLength;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &messageLength);

            std::string message {};
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

// Loads, compiles, and links a compute shader
GLuint createComputeProgram(std::string path)
{
    GLuint module = createShaderModule(path, GL_COMPUTE_SHADER);

    GLuint program = glCreateProgram();
    glAttachShader(program, module);
    {
        glLinkProgram(program);

        GLint isSuccess;
        glGetProgramiv(program, GL_LINK_STATUS, &isSuccess);

        if (!isSuccess)
        {
            GLint messageLength;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &messageLength);

            std::string message {};
            message.resize(messageLength);

            glGetProgramInfoLog(program, message.size(), nullptr, message.data());

            throw std::runtime_error("Failed to link shader program: " + message);
        }
    }
    glDetachShader(program, module);
    glDeleteShader(module);

    return program;
}

// format and type are from glTexImage3D
// format: GL_RED, GL_RED_INTEGER, GL_RG, GL_RG_INTEGER, GL_RGB, GL_RGB_INTEGER, GL_RGBA, GL_RGBA_INTEGER, GL_DEPTH_COMPONENT, GL_DEPTH_STENCIL, GL_LUMINANCE_ALPHA, GL_LUMINANCE, and GL_ALPHA
// type: GL_UNSIGNED_BYTE, GL_BYTE, GL_UNSIGNED_SHORT, GL_SHORT, GL_UNSIGNED_INT, GL_INT, GL_HALF_FLOAT, GL_FLOAT, GL_UNSIGNED_SHORT_5_6_5, GL_UNSIGNED_SHORT_4_4_4_4, GL_UNSIGNED_SHORT_5_5_5_1, GL_UNSIGNED_INT_2_10_10_10_REV, GL_UNSIGNED_INT_10F_11F_11F_REV, GL_UNSIGNED_INT_5_9_9_9_REV, GL_UNSIGNED_INT_24_8, and GL_FLOAT_32_UNSIGNED_INT_24_8_REV
GLuint create3DImage(int width, int height, int depth, GLenum format, GLenum type)
{
    GLuint img;
    glGenTextures(1, &img);

    glBindTexture(GL_TEXTURE_3D, img);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage3D(
        GL_TEXTURE_3D, 0, GL_R8UI,
        width, height, depth, // Dimensions for new mip level
        0, format, type, nullptr);

    glBindTexture(GL_TEXTURE_3D, 0);
    return img;
}

void makeNoise(GLuint image3D)
{
    // Load the make noise compute shader as needed
    if (shaderPrograms.count("makeNoise") == 0)
    {
        shaderPrograms["makeNoise"] = createComputeProgram("content/MakeNoise.compute.glsl");
    }

    glUseProgram(shaderPrograms["makeNoise"]);

    // Bind output texture to image unit 1 (write-only)
    glBindImageTexture(
        0, // Image unit index (matches binding=1)
        image3D, // Texture ID
        0, // Mip level
        GL_TRUE, // Layered (true for 3D textures)
        0, // Layer (ignored for 3D)
        GL_WRITE_ONLY, // Access qualifier
        GL_R8UI // Format
    );

    int outputWidth, outputHeight, outputDepth;

    glBindTexture(GL_TEXTURE_3D, image3D);
    glGetTexLevelParameteriv(GL_TEXTURE_3D, 0, GL_TEXTURE_WIDTH, &outputWidth);
    glGetTexLevelParameteriv(GL_TEXTURE_3D, 0, GL_TEXTURE_HEIGHT, &outputHeight);
    glGetTexLevelParameteriv(GL_TEXTURE_3D, 0, GL_TEXTURE_DEPTH, &outputDepth);
    glBindTexture(GL_TEXTURE_3D, 0);

    GLuint workGroupsX = (outputWidth + 8 - 1) / 8; // Ceiling division
    GLuint workGroupsY = (outputHeight + 8 - 1) / 8;
    GLuint workGroupsZ = (outputDepth + 8 - 1) / 8;

    int timeUniform = glGetUniformLocation(shaderPrograms["makeNoise"], "time");

    float timeValue = glfwGetTime();
    glUniform1f(timeUniform, noiseTime);

    glUniform1f(glGetUniformLocation(shaderPrograms["makeNoise"], "fillAmount"), fillAmount);
    glUniform1i(glGetUniformLocation(shaderPrograms["makeNoise"], "isRand2"), isRand2);

    glDispatchCompute(workGroupsX, workGroupsY, workGroupsZ);

    // Ensure compute shader completes
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    glBindImageTexture(
        0, // Image unit index (matches binding=1)
        0, // Texture ID
        0, // Mip level
        GL_TRUE, // Layered (true for 3D textures)
        0, // Layer (ignored for 3D)
        GL_WRITE_ONLY, // Access qualifier
        GL_R8UI // Format
    );
}

void makeMipMap(GLuint inputImage3D, GLuint outputImage3D)
{
    // Load the shader as needed
    if (shaderPrograms.count("makeMipMap") == 0)
    {
        shaderPrograms["makeMipMap"] = createComputeProgram("content/MakeMipMap.compute.glsl");
    }

    glUseProgram(shaderPrograms["makeMipMap"]);

    // Bind output texture to image unit 1 (write-only)
    glBindImageTexture(
        0, // Image unit index (matches binding=1)
        inputImage3D, // Texture ID
        0, // Mip level
        GL_TRUE, // Layered (true for 3D textures)
        0, // Layer (ignored for 3D)
        GL_READ_ONLY, // Access qualifier
        GL_R8UI // Format
    );

    // Bind output texture to image unit 1 (write-only)
    glBindImageTexture(
        1, // Image unit index (matches binding=1)
        outputImage3D, // Texture ID
        0, // Mip level
        GL_TRUE, // Layered (true for 3D textures)
        0, // Layer (ignored for 3D)
        GL_WRITE_ONLY, // Access qualifier
        GL_R8UI // Format
    );

    int outputWidth, outputHeight, outputDepth;

    glBindTexture(GL_TEXTURE_3D, outputImage3D);
    glGetTexLevelParameteriv(GL_TEXTURE_3D, 0, GL_TEXTURE_WIDTH, &outputWidth);
    glGetTexLevelParameteriv(GL_TEXTURE_3D, 0, GL_TEXTURE_HEIGHT, &outputHeight);
    glGetTexLevelParameteriv(GL_TEXTURE_3D, 0, GL_TEXTURE_DEPTH, &outputDepth);
    glBindTexture(GL_TEXTURE_3D, 0);

    GLuint workGroupsX = (outputWidth + 8 - 1) / 8; // Ceiling division
    GLuint workGroupsY = (outputHeight + 8 - 1) / 8;
    GLuint workGroupsZ = (outputDepth + 8 - 1) / 8;

    glDispatchCompute(workGroupsX, workGroupsY, workGroupsZ);

    // Ensure compute shader completes
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    // Unbind the images
    glBindImageTexture(
        0, // Image unit index (matches binding=1)
        0, // Texture ID
        0, // Mip level
        GL_TRUE, // Layered (true for 3D textures)
        0, // Layer (ignored for 3D)
        GL_WRITE_ONLY, // Access qualifier
        GL_R8UI // Format
    );

    glBindImageTexture(
        1, // Image unit index (matches binding=1)
        0, // Texture ID
        0, // Mip level
        GL_TRUE, // Layered (true for 3D textures)
        0, // Layer (ignored for 3D)
        GL_WRITE_ONLY, // Access qualifier
        GL_R8UI // Format
    );
}

std::array<float, 3> getCamDir(float theta, float phi)
{
    return {
        std::cos(theta) * std::cos(phi), std::sin(theta) * std::cos(phi), std::sin(phi)
    };
}

std::array<float, 3> getForward(float theta, float phi)
{
    return {
        std::cos(theta), std::sin(theta), 0
    };
}

std::array<float, 3> getRight(float theta, float phi)
{
    return {
        std::sin(theta), -std::cos(theta), 0
    };
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

    auto window = glfwCreateWindow(1024, 1024, "Voxel Renderer", nullptr, nullptr);
    if (window == nullptr)
    {
        throw std::runtime_error("Failed to create window");
    }

    auto window1 = std::make_shared<Window>(); // TODO: Rename this to window and use it instead of the raw pointer once the Window class is implemented
    window1->glfwWindowHandle = window;

    auto inputManager = std::make_shared<InputManager>();

    glfwGetWindowPos(window, &windowX, &windowY);
    glfwGetWindowSize(window, &windowWidth, &windowHeight);

    // Init GLEW
    glfwMakeContextCurrent(window);
    glewExperimental = true;
    if (glewInit() != GLEW_OK)
    {
        throw std::runtime_error("Failed to initialize GLEW");
    }

    // Vertex array
    GLuint emptyVertexArray;
    glGenVertexArrays(1, &emptyVertexArray);

    // Create shader program
    GLuint program = createGraphicsProgram("content/ScreenTri.vertex.glsl", "content/Raymarcher.fragment.glsl");

    // Make and fill the buffers
    GLuint occupancyMap = create3DImage(512, 512, 512, GL_RED_INTEGER, GL_UNSIGNED_BYTE);
    GLuint mipMap1 = create3DImage(128, 128, 128, GL_RED_INTEGER, GL_UNSIGNED_BYTE);
    GLuint mipMap2 = create3DImage(32, 32, 32, GL_RED_INTEGER, GL_UNSIGNED_BYTE);
    GLuint mipMap3 = create3DImage(8, 8, 8, GL_RED_INTEGER, GL_UNSIGNED_BYTE);
    GLuint mipMap4 = create3DImage(2, 2, 2, GL_RED_INTEGER, GL_UNSIGNED_BYTE);

    makeNoise(occupancyMap);
    makeMipMap(occupancyMap, mipMap1);
    makeMipMap(mipMap1, mipMap2);
    makeMipMap(mipMap2, mipMap3);
    makeMipMap(mipMap3, mipMap4);

    // Main render loop
    double theta = 0;
    double phi = 0;
    double camX = 0;
    double camY = 0;
    double camZ = 0;

    glfwSwapInterval(0); // disable vsync
    glClearColor(0, 0, 0, 0);

    if (glfwRawMouseMotionSupported())
    {
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }

    glfwGetCursorPos(window, &pastMouse[0], &pastMouse[1]);

    auto lastFrameTime = std::chrono::high_resolution_clock::now();

    int counter = 0;
    double frameTime = 0;
    while (!glfwWindowShouldClose(window))
    {
        auto now = std::chrono::high_resolution_clock::now();
        double deltaTime = std::chrono::duration<double>(now - lastFrameTime).count();
        lastFrameTime = now;
        frameTime += deltaTime;

        counter++;
        if (counter % 10 == 0)
        {
            std::cout << 10 / frameTime << std::endl;
            frameTime = 0;
        }

        glfwPollEvents();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        int width, height;
        glfwGetWindowSize(window, &width, &height);
        if (!invalidateMouse)
        {
            theta -= (mousePos[0] - pastMouse[0]) * 0.002;
            phi -= (mousePos[1] - pastMouse[1]) * 0.002;
            phi = std::min(std::max(phi, -3.1415926589 / 2), 3.1415926589 / 2);
        }
        else
        {
            invalidateMouse = false;
        }

        pastMouse[0] = mousePos[0];
        pastMouse[1] = mousePos[1];

        auto right = getRight(theta, phi);
        auto forward = getForward(theta, phi);
        auto camDirection = getCamDir(theta, phi);

        if (heldKeys.count(GLFW_KEY_A))
        {
            camX -= right[0] * deltaTime * std::pow(2, mouseWheel * 0.1);
            camY -= right[1] * deltaTime * std::pow(2, mouseWheel * 0.1);
            camZ -= right[2] * deltaTime * std::pow(2, mouseWheel * 0.1);
        }
        if (heldKeys.count(GLFW_KEY_D))
        {
            camX += right[0] * deltaTime * std::pow(2, mouseWheel * 0.1);
            camY += right[1] * deltaTime * std::pow(2, mouseWheel * 0.1);
            camZ += right[2] * deltaTime * std::pow(2, mouseWheel * 0.1);
        }

        if (heldKeys.count(GLFW_KEY_W))
        {
            camX += forward[0] * deltaTime * std::pow(2, mouseWheel * 0.1);
            camY += forward[1] * deltaTime * std::pow(2, mouseWheel * 0.1);
            camZ += forward[2] * deltaTime * std::pow(2, mouseWheel * 0.1);
        }
        if (heldKeys.count(GLFW_KEY_S))
        {
            camX -= forward[0] * deltaTime * std::pow(2, mouseWheel * 0.1);
            camY -= forward[1] * deltaTime * std::pow(2, mouseWheel * 0.1);
            camZ -= forward[2] * deltaTime * std::pow(2, mouseWheel * 0.1);
        }

        if (heldKeys.count(GLFW_KEY_SPACE))
        {
            camZ += deltaTime * std::pow(2, mouseWheel * 0.1);
        }
        if (heldKeys.count(GLFW_KEY_LEFT_SHIFT))
        {
            camZ -= deltaTime * std::pow(2, mouseWheel * 0.1);
        }

        if (heldKeys.count(GLFW_KEY_E))
        {
            noiseTime += deltaTime;
            makeNoise(occupancyMap);
            makeMipMap(occupancyMap, mipMap1);
            makeMipMap(mipMap1, mipMap2);
            makeMipMap(mipMap2, mipMap3);
            makeMipMap(mipMap3, mipMap4);
        }

        if (remakeNoise)
        {
            // The noise time should not be incremented here
            makeNoise(occupancyMap);
            makeMipMap(occupancyMap, mipMap1);
            makeMipMap(mipMap1, mipMap2);
            makeMipMap(mipMap2, mipMap3);
            makeMipMap(mipMap3, mipMap4);
            remakeNoise = false;
        }

        {
            glUseProgram(program);
            glBindVertexArray(emptyVertexArray);

            glBindImageTexture(
                0, // Image unit index (matches binding=1)
                occupancyMap, // Texture ID
                0, // Mip level
                GL_TRUE, // Layered (true for 3D textures)
                0, // Layer (ignored for 3D)
                GL_READ_ONLY, // Access qualifier
                GL_R8UI // Format
            );

            glBindImageTexture(
                1, // Image unit index (matches binding=1)
                mipMap1, // Texture ID
                0, // Mip level
                GL_TRUE, // Layered (true for 3D textures)
                0, // Layer (ignored for 3D)
                GL_READ_ONLY, // Access qualifier
                GL_R8UI // Format
            );

            glBindImageTexture(
                2, // Image unit index (matches binding=1)
                mipMap2, // Texture ID
                0, // Mip level
                GL_TRUE, // Layered (true for 3D textures)
                0, // Layer (ignored for 3D)
                GL_READ_ONLY, // Access qualifier
                GL_R8UI // Format
            );

            glBindImageTexture(
                3, // Image unit index (matches binding=1)
                mipMap3, // Texture ID
                0, // Mip level
                GL_TRUE, // Layered (true for 3D textures)
                0, // Layer (ignored for 3D)
                GL_READ_ONLY, // Access qualifier
                GL_R8UI // Format
            );

            glBindImageTexture(
                4, // Image unit index (matches binding=1)
                mipMap4, // Texture ID
                0, // Mip level
                GL_TRUE, // Layered (true for 3D textures)
                0, // Layer (ignored for 3D)
                GL_READ_ONLY, // Access qualifier
                GL_R8UI // Format
            );

            int camPos = glGetUniformLocation(program, "camPos");
            glUniform3f(camPos, camX, camY, camZ);

            int camDir = glGetUniformLocation(program, "camDir");
            glUniform3f(camDir, camDirection[0], camDirection[1], camDirection[2]);

            int resolution = glGetUniformLocation(program, "resolution");
            glUniform2f(resolution, width, height);

            glUniform1i(glGetUniformLocation(program, "isWorkload"), isWorkload);

            glDrawArrays(GL_TRIANGLES, 0, 3);

            glUseProgram(0);
            glBindVertexArray(0);
        }

        glfwSwapBuffers(window);
    }

    return 0;
}
