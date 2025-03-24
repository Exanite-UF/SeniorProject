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
    inline static std::vector<std::function<void()>> singletonDestroyFunctions {};

public:
    // Cleanup all singletons, usually when the program is exiting
    static void destroyAllSingletons()
    {
        // Disable creation of more singletons
        canCreate = false;

        // Call cleanup in reverse order
        for (int i = singletonCleanupFunctions.size() - 1; i >= 0; --i)
        {
            singletonCleanupFunctions.at(i)();
        }

        singletonCleanupFunctions.clear();

        // Call destroy in reverse order
        for (int i = singletonDestroyFunctions.size() - 1; i >= 0; --i)
        {
            singletonDestroyFunctions.at(i)();
        }

        singletonDestroyFunctions.clear();
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

        static void addDestroyFunction(const std::function<void()>& destroy)
        {
            singletonDestroyFunctions.push_back(destroy);
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

    virtual void onSingletonDestroy() {}

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
        Assert::isTrue(SingletonManager::Internal<T>::getCanCreate(), "SingletonManager::destroyAllSingletons() has been called, can no longer create new singletons");

        instance = new T();

        SingletonManager::Internal<T>::addCleanupFunction([&]()
            {
                reinterpret_cast<Singleton*>(instance)->onSingletonDestroy();
            });

        SingletonManager::Internal<T>::addDestroyFunction([&]()
            {
                delete instance;
            });
    }

    return *instance;
}
