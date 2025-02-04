#pragma once

#include <functional>

template <typename T>
struct Event;

template <typename TReturnType, typename... TArgs>
struct Event<TReturnType(TArgs...)>
{
private:
    std::vector<std::function<TReturnType(TArgs...)>> listeners;

public:
    std::function<TReturnType(TArgs...)> subscribe(std::function<TReturnType(TArgs...)> listener);
    void unsubscribe(std::function<TReturnType(TArgs...)> listener);

    void raise(TArgs... args);

    int getCount();
};

template <typename TReturnType, typename... TArgs>
std::function<TReturnType(TArgs...)> Event<TReturnType(TArgs...)>::subscribe(std::function<TReturnType(TArgs...)> listener)
{
    listeners.push_back(listener);

    return listener;
}

template <typename TReturnType, typename... TArgs>
void Event<TReturnType(TArgs...)>::unsubscribe(std::function<TReturnType(TArgs...)> listener)
{
    // listeners.erase(listener);
}

template <typename TReturnType, typename... TArgs>
void Event<TReturnType(TArgs...)>::raise(TArgs... args)
{
    for (auto listener : listeners)
    {
        listener(args...);
    }
}

template <typename TReturnType, typename... TArgs>
int Event<TReturnType(TArgs...)>::getCount()
{
    return listeners.size();
}
