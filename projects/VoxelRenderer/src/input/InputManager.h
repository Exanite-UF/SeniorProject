#pragma once

#include <GL/glew.h>

#include <GLFW/glfw3.h>
#include <glm/vec2.hpp>
#include <memory>
#include <unordered_set>

#include <src/utilities/NonCopyable.h>
#include <src/windowing/Window.h>

// Stores a snapshot of the input state
struct InputState
{
    std::unordered_set<int> heldKeys {}; // Keyboard keys
    std::unordered_set<int> heldButtons {}; // Mouse buttons

    glm::vec2 mousePosition {};
    glm::vec2 mouseScroll {};

    bool isKeyHeld(int key) const;
    bool isButtonHeld(int button) const;
};

// Used to access the state of the input
// Provides abstractions to see changes in state between the last two updates
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

// This manages the recording of the input state
class InputManager : public NonCopyable
{
private:
    // Accumulates changes to the input state, so that it always has the most recent changes to the input.
    // These most recent inputs are then recorded in the Input object every update.
    InputState next {};
    std::shared_ptr<Window> window;

public:
    std::unique_ptr<Input> input;
    bool cursorEnteredThisFrame = true;

    explicit InputManager(std::shared_ptr<Window> window);

    void update();

    void onKey(Window* window, int key, int scancode, int action, int mods);
    void onMouseButton(Window* window, int button, int action, int mods);
    void onCursorPos(Window* window, double xpos, double ypos);
    void onScroll(Window* window, double xoffset, double yoffset);
    void onCursorEnter(Window* window, int entered);
};
