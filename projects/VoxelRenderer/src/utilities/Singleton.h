#pragma once

#include <functional>
#include <vector>

#include <src/utilities/Assert.h>
#include <src/utilities/NonCopyable.h>

template <class T>
class Singleton;

class SingletonManager
{
private:
    inline static bool canCreate = true;
    inline static std::vector<std::function<void()>> singletonCleanupFunctions {};

public:
    // Cleanup all singletons, usually when the program is exiting
    static void destroyAllSingletons()
    {
        canCreate = false;

        // Call in reverse order
        for (int i = singletonCleanupFunctions.size() - 1; i >= 0; --i)
        {
            singletonCleanupFunctions.at(i)();
        }

        singletonCleanupFunctions.clear();
    }

    // This makes it so that singletons can access private APIs
    // We cannot directly friend the non-templated Singleton class
    template <class T>
    class Internal
    {
        friend class Singleton<T>;

    private:
        static bool getCanCreate()
        {
            return canCreate;
        }

        static void addCleanupFunction(const std::function<void()>& cleanup)
        {
            singletonCleanupFunctions.push_back(cleanup);
        }
    };
};

template <class T>
class Singleton : public NonCopyable
{
private:
    static T* instance;

protected:
    Singleton() = default;
    ~Singleton() override = default;

public:
    static T& getInstance();
};

template <class T>
T* Singleton<T>::instance = nullptr;

template <class T>
T& Singleton<T>::getInstance()
{
    Assert::isTrue(SingletonManager::Internal<T>::getCanCreate(), "SingletonManager::destroyAllSingletons() has been called, can no longer access singletons");

    if (instance == nullptr)
    {
        instance = new T();
        SingletonManager::Internal<T>::addCleanupFunction([&]()
            {
                delete instance;
            });
    }

    return *instance;
}
