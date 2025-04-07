#include "Assert.h"

#include <src/utilities/Log.h>
#include <stdexcept>

void Assert::isTrue(const bool condition, const std::string& errorMessage)
{
    if (!condition)
    {
        Log::error("Assert failed: " + errorMessage);
        throw std::runtime_error("Assert failed: " + errorMessage);
    }
}
