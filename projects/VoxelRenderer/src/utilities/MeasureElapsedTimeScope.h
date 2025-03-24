#pragma once
#include <chrono>

class MeasureElapsedTimeScope
{
private:
    std::string name;
    std::chrono::high_resolution_clock::time_point start;

    double minTimeToPrintMs = 0;

public:
    explicit MeasureElapsedTimeScope(const std::string& name, double minTimeToPrintMs = 0);
    ~MeasureElapsedTimeScope();
};
