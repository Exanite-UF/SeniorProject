#pragma once

#include <functional>
#include <memory>
#include <queue>

#include <src/utilities/Event.h>

// Based on http://nercury.github.io/c++/interesting/2016/02/22/weak_ptr-and-event-cleanup.html
template <typename... TArgs>
class BufferedEvent : public Event<TArgs...>
{
private:
    std::queue<std::tuple<TArgs...>> bufferedEvents;

public:
    void raise(TArgs... args) override;
    void flush();
};

template <typename... TArgs>
void BufferedEvent<TArgs...>::raise(TArgs... args)
{
    bufferedEvents.push(std::tuple<TArgs...>(args...));
}

template <typename... TArgs>
void BufferedEvent<TArgs...>::flush()
{
    while (!bufferedEvents.empty())
    {
        std::tuple<TArgs...> arguments = bufferedEvents.front();
        bufferedEvents.pop();

        std::apply([&, this](auto&&... args)
            {
                Event<TArgs...>::raise(std::forward<decltype(args)>(args)...);
            },
            arguments);
    }
}
