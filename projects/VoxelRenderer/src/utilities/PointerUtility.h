#pragma once

#include <memory>
#include <src/utilities/Assert.h>

class PointerUtility
{
public:
    template <typename T>
    static std::shared_ptr<T> safeLock(const std::weak_ptr<T>& pointer, const std::string& errorMessage = "Weak pointer has expired")
    {
        auto shared = pointer.lock();
        Assert::isTrue(!!shared, errorMessage);

        return shared;
    }
};
