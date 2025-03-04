#pragma once
#include <chrono>

class MeasureElapsedTimeScope
{
private:
    std::string name;
    std::chrono::time_point<std::chrono::system_clock> start;

public:
    MeasureElapsedTimeScope(const std::string& name);
    ~MeasureElapsedTimeScope();
};
