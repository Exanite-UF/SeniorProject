#include <src/input/InputManager.h>

bool InputState::isKeyHeld(const int key) const
{
    return heldKeys.contains(key);
}

bool InputState::isButtonHeld(const int button) const
{
    return heldButtons.contains(button);
}

glm::vec2 Input::getMousePosition() const
{
    return current.mousePosition;
}

glm::vec2 Input::getMouseDelta() const
{
    return current.mousePosition - previous.mousePosition;
}

glm::vec2 Input::getMouseScroll() const
{
    return current.mouseScroll;
}

bool Input::isKeyHeld(const int key) const
{
    return current.isKeyHeld(key);
}

bool Input::isKeyPressed(const int key) const
{
    return current.isKeyHeld(key) && !previous.isKeyHeld(key);
}

bool Input::isKeyReleased(const int key) const
{
    return !current.isKeyHeld(key) && previous.isKeyHeld(key);
}

bool Input::isButtonHeld(const int button) const
{
    return current.isButtonHeld(button);
}

bool Input::isButtonPressed(const int button) const
{
    return current.isButtonHeld(button) && !previous.isButtonHeld(button);
}

bool Input::isButtonReleased(const int button) const
{
    return !current.isButtonHeld(button) && previous.isButtonHeld(button);
}

InputManager::InputManager()
{
    input = std::make_unique<Input>();
}

void InputManager::update()
{
    input->previous = input->current;
    input->current = next;

    next.mouseScroll = glm::vec2(0);
}

void InputManager::onKey(GLFWwindow* window, int key, int scancode, int action, int mods)
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

void InputManager::onMouseButton(GLFWwindow* window, int button, int action, int mods)
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

void InputManager::onCursorPos(GLFWwindow* window, double xpos, double ypos)
{
    next.mousePosition.x = xpos;
    next.mousePosition.y = ypos;
}

void InputManager::onScroll(GLFWwindow* window, double xoffset, double yoffset)
{
    next.mouseScroll.x = xoffset;
    next.mouseScroll.y = yoffset;
}
