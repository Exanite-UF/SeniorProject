#pragma once

#include <functional>
#include <memory>

// Based on http://nercury.github.io/c++/interesting/2016/02/22/weak_ptr-and-event-cleanup.html
template <typename... TArgs>
class Event
{
private:
    // A list of all subscriptions, both externally owned and internally owned.
    std::vector<std::weak_ptr<std::function<void(TArgs...)>>> subscriptions;

    // A list of internally owned subscriptions.
    std::vector<std::shared_ptr<std::function<void(TArgs...)>>> ownedSubscriptions;

    int recursionCount = 0;

public:
    // Add a function to be called when this event is raised.
    // The returned shared pointer acts as a handle to the event subscription. The caller of the method owns the event subscription.
    //
    // When the shared pointer is reset or destroyed, the event subscription will be removed.
    // Use this when you want to eventually unsubscribe from the event.
    std::shared_ptr<std::function<void(TArgs...)>> subscribe(std::function<void(TArgs...)> listener);

    // Add a function to be called when this event is raised.
    // Unlike the subscribe() method, the event subscription is owned by the Event instance itself.
    //
    // Use this when you don't intend to unsubscribe from the event.
    // Event subscriptions will still be cleaned up when the event class is destroyed.
    void subscribePermanently(std::function<void(TArgs...)> listener);

    // Clear all event subscriptions that were added through the subscribePermanently() method.
    void clearPermanentSubscriptions();

    // Raise an event, causing subscribers to be notified.
    virtual void raise(TArgs... args);
};

template <typename... TArgs>
std::shared_ptr<std::function<void(TArgs...)>> Event<TArgs...>::subscribe(std::function<void(TArgs...)> listener)
{
    auto shared = std::make_shared<std::function<void(TArgs...)>>(listener);
    subscriptions.push_back(shared);

    return shared;
}

template <typename... TArgs>
void Event<TArgs...>::subscribePermanently(std::function<void(TArgs...)> listener)
{
    ownedSubscriptions.push_back(subscribe(listener));
}

template <typename... TArgs>
void Event<TArgs...>::clearPermanentSubscriptions()
{
    ownedSubscriptions.clear();
}

template <typename... TArgs>
void Event<TArgs...>::raise(TArgs... args)
{
    // Track recursion count because events can raise themselves
    recursionCount++;
    try
    {
        for (std::weak_ptr<std::function<void(TArgs...)>> listener : subscriptions)
        {
            auto shared = listener.lock();
            if (shared)
            {
                (*shared)(args...);
            }
        }
    }
    catch (...)
    {
        recursionCount--;
        throw;
    };

    // Only remove entries from listeners list when no events are running
    if (recursionCount == 0)
    {
        for (int i = subscriptions.size() - 1; i >= 0; --i)
        {
            if (subscriptions.at(i).expired())
            {
                subscriptions.erase(subscriptions.begin() + i);
            }
        }
    }
}
