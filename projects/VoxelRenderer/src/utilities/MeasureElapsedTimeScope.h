#pragma once

#include <chrono>

#include <src/utilities/Log.h>

class MeasureElapsedTimeScope
{
private:
    std::string name;
    Log::LogLevel logLevel;
    double minTimeToPrintMs = 0;

    std::chrono::high_resolution_clock::time_point start;

public:
    explicit MeasureElapsedTimeScope(const std::string& name, double minTimeToPrintMs = 0);
    explicit MeasureElapsedTimeScope(const std::string& name, Log::LogLevel logLevel, double minTimeToPrintMs = 0);

    ~MeasureElapsedTimeScope();
};
