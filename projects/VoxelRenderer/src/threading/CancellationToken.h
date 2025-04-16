#pragma once

#include <memory>

class CancellationTokenSource;

class CancellationToken
{
    friend class CancellationTokenSource;

private:
    bool wasCreated = false;
    std::weak_ptr<bool> token {}; // Bool is unused

    explicit CancellationToken(const std::weak_ptr<bool>& token);

public:
    CancellationToken();

    bool isCancellationRequested() const;
};

class CancellationTokenSource
{
private:
    std::shared_ptr<bool> token = std::make_shared<bool>(false); // Bool is unused

public:
    void requestCancellation();
    CancellationToken getCancellationToken() const;
};
