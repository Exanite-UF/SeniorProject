#pragma once

#include <src/utilities/NonCopyable.h>

template <class T>
class Singleton : public NonCopyable
{
private:
    static T* instance;

protected:
    Singleton() = default;
    ~Singleton() = default;

public:
    static T& getInstance();
};

template <class T>
T* Singleton<T>::instance = nullptr;

template <class T>
T& Singleton<T>::getInstance()
{
    if (instance == nullptr)
    {
        instance = new T();
    }

    return *instance;
}
