#include "Assert.h"

#include <stdexcept>

void Assert::isTrue(const bool condition, const std::string& errorMessage)
{
    if (!condition)
    {
        throw std::runtime_error("Assert failed: " + errorMessage);
    }
}
