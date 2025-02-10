// Include GLEW before OpenGL and GLFW
#include <GL/glew.h>

#include <GLFW/glfw3.h>

#include "VoxelRendererProgram.h"

int main()
{
    VoxelRendererProgram program;
    program.run();

    return 0;
}
