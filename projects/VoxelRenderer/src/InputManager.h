#pragma once

#include <glm/vec2.hpp>
#include <unordered_set>

struct InputState
{
    std::unordered_set<int> heldKeys {};
    std::unordered_set<int> heldButtons {};

    glm::vec2 mousePosition {};
    glm::vec2 mouseScroll {};

    bool isKeyHeld(const int key) const
    {
        return heldKeys.contains(key);
    }
};

struct Input
{
    InputState current {};
    InputState previous {};

    glm::vec2 getMousePosition() const
    {
        return current.mousePosition;
    }

    glm::vec2 getMouseDelta() const
    {
        return current.mousePosition - previous.mousePosition;
    }

    glm::vec2 getMouseScroll() const
    {
        return current.mouseScroll;
    }

    bool isKeyHeld(const int key) const
    {
        return current.isKeyHeld(key);
    }

    bool isKeyPressed(const int key) const
    {
        return current.isKeyHeld(key) && !previous.isKeyHeld(key);
    }

    bool isKeyReleased(const int key) const
    {
        return !current.isKeyHeld(key) && previous.isKeyHeld(key);
    }
};

class InputManager
{
private:
public:
    Input input {};
};
