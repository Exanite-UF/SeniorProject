#pragma once
#include <atomic>

// This is meant for debugging and is not fully thread safe
template <class T>
class CountInstances
{
private:
    static std::atomic<int> instanceCount;

public:
    CountInstances();

    virtual ~CountInstances();

    static int getInstanceCount();
};

template <class T>
std::atomic<int> CountInstances<T>::instanceCount = 0;

template <class T>
CountInstances<T>::CountInstances()
{
    instanceCount++;
}

template <class T>
CountInstances<T>::~CountInstances()
{
    instanceCount--;
}

template <class T>
int CountInstances<T>::getInstanceCount()
{
    return instanceCount;
}
