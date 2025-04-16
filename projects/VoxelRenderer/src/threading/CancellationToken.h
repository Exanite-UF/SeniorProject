#pragma once

class CancellationToken
{
    // Intentionally empty class
    // It is possible to implement the functionality here, but we can use shared and weak pointers instead

    // Have the task owner use a std::shared_ptr<CancellationToken>
    // Have the task use a std::weak_ptr<CancellationToken> and check if it has expired

    // Note: You have to check if the weak pointer has expired, not if it is valid
    // This is because we only want to cancel if a CancellationToken was created and has expired
    // We don't want to cancel when there was no CancellationToken to begin with
};
