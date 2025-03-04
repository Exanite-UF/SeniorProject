#pragma once
#include <chrono>

class MeasureElapsedTimeScope
{
private:
    std::string name;
    std::chrono::high_resolution_clock::time_point start;

public:
    MeasureElapsedTimeScope(const std::string& name);
    ~MeasureElapsedTimeScope();
};
