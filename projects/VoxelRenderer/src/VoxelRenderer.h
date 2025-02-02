#pragma once

#include <filesystem>
#include <iostream>
#include <string>

class VoxelRenderer
{
public:
    static void log(const std::string& value = "");

    static void checkForContentFolder();

    static void assertIsTrue(bool condition, const std::string& errorMessage);

    static void runStartupTests();
};
