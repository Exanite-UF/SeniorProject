#pragma once

// Inherit from this to make a class non-copyable
class NonCopyable
{
public:
    // Allow creation and destruction
    NonCopyable() = default;
    virtual ~NonCopyable() = default;

    // Deny copies
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;
};
