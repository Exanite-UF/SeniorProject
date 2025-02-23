// Include GLEW before OpenGL and GLFW
#include <GL/glew.h>

#include <GLFW/glfw3.h>

#include <src/Program.h>
#include <src/utilities/Log.h>

int main()
{
    try
    {
        Program program;
        program.run();
    }
    catch (const std::runtime_error& e)
    {
        Log::log("Program crashed: Runtime error: " + std::string(e.what()));

        return 1;
    }

    return 0;
}
