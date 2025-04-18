#include <src/threading/CancellationToken.h>

CancellationToken::CancellationToken(const std::weak_ptr<bool>& token)
{
    wasCreated = true;
    this->token = token;
}

CancellationToken::CancellationToken() = default;

bool CancellationToken::isCancellationRequested() const
{
    return wasCreated && token.expired();
}

void CancellationTokenSource::requestCancellation()
{
    token.reset();
}

CancellationToken CancellationTokenSource::getCancellationToken() const
{
    return CancellationToken(token);
}
