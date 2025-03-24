#include "MeasureElapsedTimeScope.h"

#include <chrono>

#include <src/utilities/Log.h>

MeasureElapsedTimeScope::MeasureElapsedTimeScope(const std::string& name, const double minTimeToPrintMs)
    : MeasureElapsedTimeScope(name, Log::Information, minTimeToPrintMs)
{
}

MeasureElapsedTimeScope::MeasureElapsedTimeScope(const std::string& name, const Log::LogLevel logLevel,
    const double minTimeToPrintMs)
{
    this->name = name;
    this->minTimeToPrintMs = minTimeToPrintMs;
    this->logLevel = logLevel;

    start = std::chrono::high_resolution_clock::now();
}

MeasureElapsedTimeScope::~MeasureElapsedTimeScope()
{
    std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
    auto elapsedTimeMs = std::chrono::duration<double>(now - start).count() * 1000;

    if (elapsedTimeMs > minTimeToPrintMs)
    {
        Log::write(logLevel, "Elapsed time for scope '" + name + "': " + std::to_string(elapsedTimeMs) + " ms");
    }
}
