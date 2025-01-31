#pragma once

#include <GLFW/glfw3.h>
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

    bool isButtonHeld(const int button) const
    {
        return heldButtons.contains(button);
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

    bool isButtonHeld(const int button) const
    {
        return current.isButtonHeld(button);
    }

    bool isButtonPressed(const int button) const
    {
        return current.isButtonHeld(button) && !previous.isButtonHeld(button);
    }

    bool isButtonReleased(const int button) const
    {
        return !current.isButtonHeld(button) && previous.isButtonHeld(button);
    }
};

class InputManager
{
private:
    InputState next {};

public:
    Input input {};

    void update()
    {
        input.previous = input.current;
        input.current = next;

        next.mouseScroll = glm::vec2(0);
    }

    void onKey(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        if (action == GLFW_PRESS)
        {
            next.heldKeys.insert(key);
        }

        if (action == GLFW_RELEASE)
        {
            next.heldKeys.erase(key);
        }
    }

    void onMouseButton(GLFWwindow* window, int button, int action, int mods)
    {
        if (action == GLFW_PRESS)
        {
            next.heldButtons.insert(button);
        }

        if (action == GLFW_RELEASE)
        {
            next.heldButtons.erase(button);
        }
    }

    void onCursorPos(GLFWwindow* window, double xpos, double ypos)
    {
        next.mousePosition.x = xpos;
        next.mousePosition.y = ypos;
    }

    void onScroll(GLFWwindow* window, double xoffset, double yoffset)
    {
        next.mouseScroll.x = xoffset;
        next.mouseScroll.y = yoffset;
    }
};
