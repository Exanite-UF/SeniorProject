#pragma once

#include <algorithm>
#include <future>
#include <vector>

template <typename T>
class PendingTasks
{
private:
    std::vector<std::shared_future<T>> pending {};
    std::mutex mutex {};

public:
    explicit PendingTasks();
    explicit PendingTasks(const std::vector<std::shared_future<T>>& pending);

    void addPending(const std::shared_future<T>& pending);
    void addPending(const std::vector<std::shared_future<T>>& pending);

    // Removes finished tasks and only returns tasks that are still pending
    std::vector<std::shared_future<T>> getPending();

    void waitForPending();
};

template <typename T>
PendingTasks<T>::PendingTasks() = default;

template <typename T>
PendingTasks<T>::PendingTasks(const std::vector<std::shared_future<T>>& pending)
{
    this->pending = pending;
}

template <typename T>
void PendingTasks<T>::addPending(const std::shared_future<T>& pending)
{
    std::lock_guard lock(mutex);

    this->pending.push_back(pending);
}

template <typename T>
void PendingTasks<T>::addPending(const std::vector<std::shared_future<T>>& pending)
{
    std::lock_guard lock(mutex);

    this->pending.insert(this->pending.end(), pending.begin(), pending.end());
}

template <typename T>
std::vector<std::shared_future<T>> PendingTasks<T>::getPending()
{
    std::lock_guard lock(mutex);

    // Remove completed tasks
    auto removeIterator = std::remove_if(pending.begin(), pending.end(), [](const std::shared_future<T>& task)
        {
            return task.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
        });

    pending.erase(removeIterator, pending.end());

    // Return copy
    return pending;
}

template <typename T>
void PendingTasks<T>::waitForPending()
{
    // Must use while statement since more tasks could have been added during the wait
    while (true)
    {
        // Use a copy so we don't have to lock the original vector
        auto copy = getPending();
        if (copy.empty())
        {
            return;
        }

        // Wait for all known tasks
        for (auto dependency : copy)
        {
            dependency.wait();
        }
    }
}
