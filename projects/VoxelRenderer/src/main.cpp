// Include GLEW before OpenGL and GLFW
#include <GL/glew.h>

#include <GLFW/glfw3.h>

#include <iostream>
#include <src/Program.h>
#include <src/utilities/Log.h>
#include <stdexcept>

int main()
{
    try
    {
        Program program;
        program.run();
    }
    catch (const std::runtime_error& e)
    {
        // TODO: No idea why this prints gibberish on Windows
        Log::log("Program crashed: Runtime error: " + std::string(e.what()));

        return 1;
    }
    catch (...)
    {
        Log::log("Program crashed: Unknown error");

        return 1;
    }

    return 0;
}
