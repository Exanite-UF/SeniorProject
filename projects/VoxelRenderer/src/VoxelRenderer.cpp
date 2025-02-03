#include "VoxelRenderer.h"
#include "TupleHasher.h"
#include <string>
#include <tuple>
#include <unordered_map>

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

    auto key = std::make_tuple("hello", 1);
    std::unordered_map<std::tuple<std::string, int>, int, TupleHasher<std::tuple<std::string, int>>> map;
    assertIsTrue(!map.contains(key), "Incorrect map implementation: Map should be empty");
    map[key] = 1;
    assertIsTrue(map.contains(key), "Incorrect map implementation: Map should contain key that was inserted");
}
