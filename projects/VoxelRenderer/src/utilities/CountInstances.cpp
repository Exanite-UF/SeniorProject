#include "CountInstances.h"

#include <atomic>

std::atomic<int> CountInstances::instanceCount = 0;

CountInstances::CountInstances()
{
    instanceCount++;
}

CountInstances::~CountInstances()
{
    instanceCount--;
}

int CountInstances::getInstanceCount()
{
    return instanceCount;
}
