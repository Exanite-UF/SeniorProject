#pragma once

// Include GLEW before OpenGL and GLFW
#include <GL/glew.h>

#include <GLFW/glfw3.h>

#include <string>

// WASD Space Shift = movement
// q = capture mouse
// f = fullscreen toggle
// e = progress through the noise's time
// t = change between types of noise
// scroll = change move speed
// CTRL + scroll = change noise fill
class VoxelRendererProgram
{
public:
    bool isWorkload = false; // View toggle
    bool isRand2 = true; // Noise type toggle
    float fillAmount = 0.6;
    bool remakeNoise = false;

    double noiseTime = 0;

    GLuint raymarcherGraphicsProgram;
    GLuint makeNoiseComputeProgram;
    GLuint makeMipMapComputeProgram;
    GLuint assignMaterialComputeProgram;

    VoxelRendererProgram();
    VoxelRendererProgram(const VoxelRendererProgram&) = delete;
    VoxelRendererProgram& operator=(const VoxelRendererProgram&) = delete;

    ~VoxelRendererProgram();

    void run();

    static void log(const std::string& value = "");

    static void checkForContentFolder();

    static void assertIsTrue(bool condition, const std::string& errorMessage);

    static void runStartupTests();
};
