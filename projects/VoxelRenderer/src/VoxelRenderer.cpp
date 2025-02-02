#include "VoxelRenderer.h"

void VoxelRenderer::log(const std::string& value)
{
    std::cout << value + "\n"
              << std::flush;
}

void VoxelRenderer::checkForContentFolder()
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

void VoxelRenderer::assertIsTrue(const bool condition, const std::string& errorMessage)
{
    if (!condition)
    {
        throw std::runtime_error("Assert failed: " + errorMessage);
    }
}

void VoxelRenderer::runStartupTests()
{
    checkForContentFolder();
    assertIsTrue(NULL == nullptr, "Unsupported compiler: NULL must equal nullptr");
}
