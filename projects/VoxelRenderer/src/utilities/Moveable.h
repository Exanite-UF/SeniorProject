#pragma once

// Inherit from this to make a class moveable
class Moveable
{
public:
    // Allow creation and destruction
    Moveable() = default;
    virtual ~Moveable() = default;

    // Allow moves
    Moveable(Moveable&&) = default;
    Moveable& operator=(Moveable&&) = default;
};
