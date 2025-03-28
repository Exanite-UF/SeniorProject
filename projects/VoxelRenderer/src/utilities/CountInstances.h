#pragma once
#include <atomic>

// This is meant for debugging and is not fully thread safe
class CountInstances
{
private:
    static std::atomic<int> instanceCount;

public:
    CountInstances();
    ~CountInstances();

    static int getInstanceCount();
};
