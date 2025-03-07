#pragma once

// Inherit from this to make a class non-copyable
class NonCopyable
{
public:
    virtual ~NonCopyable() = default;

    // Allow creation
    NonCopyable() = default;

    // Deny copies
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;
};
