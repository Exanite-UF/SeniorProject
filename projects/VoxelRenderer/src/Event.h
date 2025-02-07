#pragma once

#include <functional>

// Based on http://nercury.github.io/c++/interesting/2016/02/22/weak_ptr-and-event-cleanup.html
template <typename... TArgs>
class Event
{
private:
    std::vector<std::weak_ptr<std::function<void(TArgs...)>>> listeners;
    int recursionCount = 0;

public:
    std::shared_ptr<std::function<void(TArgs...)>> subscribe(std::function<void(TArgs...)> listener);

    void raise(TArgs... args);
};

template <typename... TArgs>
std::shared_ptr<std::function<void(TArgs...)>> Event<TArgs...>::subscribe(std::function<void(TArgs...)> listener)
{
    auto shared = std::make_shared<std::function<void(TArgs...)>>(listener);
    listeners.push_back(shared);

    return shared;
}

template <typename... TArgs>
void Event<TArgs...>::raise(TArgs... args)
{
    // Track recursion count because events can raise themselves
    recursionCount++;
    for (std::weak_ptr<std::function<void(TArgs...)>> listener : listeners)
    {
        auto shared = listener.lock();
        if (shared)
        {
            (*shared)(args...);
        }
    }
    recursionCount--;

    // Only remove entries from listeners list when no events are running
    if (recursionCount == 0)
    {
        for (int i = listeners.size() - 1; i >= 0; --i)
        {
            if (listeners.at(i).expired())
            {
                listeners.erase(listeners.begin() + i);
            }
        }
    }
}
