#include "MeasureElapsedTimeScope.h"

#include <chrono>

#include <src/utilities/Log.h>

MeasureElapsedTimeScope::MeasureElapsedTimeScope(const std::string& name)
{
    this->name = name;
    start = std::chrono::high_resolution_clock::now();
}

MeasureElapsedTimeScope::~MeasureElapsedTimeScope()
{
    std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
    auto elapsedTime = std::chrono::duration<double>(now - start).count();
    Log::log("Elapsed time for scope '" + name + "': " + std::to_string(elapsedTime * 1000) + " ms");
}
