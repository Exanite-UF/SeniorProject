#include "MeasureElapsedTimeScope.h"

#include <chrono>

#include <src/utilities/Log.h>

MeasureElapsedTimeScope::MeasureElapsedTimeScope(const std::string& name, double minTimeToPrintMs)
{
    this->name = name;
    this->minTimeToPrintMs = minTimeToPrintMs;

    start = std::chrono::high_resolution_clock::now();
}

MeasureElapsedTimeScope::~MeasureElapsedTimeScope()
{
    std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
    auto elapsedTimeMs = std::chrono::duration<double>(now - start).count() * 1000;

    if (elapsedTimeMs > minTimeToPrintMs)
    {
        Log::log("Elapsed time for scope '" + name + "': " + std::to_string(elapsedTimeMs) + " ms");
    }
}
