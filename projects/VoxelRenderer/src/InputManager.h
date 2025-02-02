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

    bool isKeyHeld(int key) const;
    bool isButtonHeld(int button) const;
};

struct Input
{
    InputState current {};
    InputState previous {};

    glm::vec2 getMousePosition() const;
    glm::vec2 getMouseDelta() const;
    glm::vec2 getMouseScroll() const;

    bool isKeyHeld(int key) const;
    bool isKeyPressed(int key) const;
    bool isKeyReleased(int key) const;

    bool isButtonHeld(int button) const;
    bool isButtonPressed(int button) const;
    bool isButtonReleased(int button) const;
};

class InputManager
{
private:
    InputState next {};

public:
    Input input {};

    void update();

    void onKey(GLFWwindow* window, int key, int scancode, int action, int mods);
    void onMouseButton(GLFWwindow* window, int button, int action, int mods);
    void onCursorPos(GLFWwindow* window, double xpos, double ypos);
    void onScroll(GLFWwindow* window, double xoffset, double yoffset);
};
