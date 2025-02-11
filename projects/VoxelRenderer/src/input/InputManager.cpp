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

InputManager::InputManager(std::shared_ptr<Window> window)
{
    this->window = window;
    input = std::make_unique<Input>();

    eventSubscriptions.push_back(window->keyEvent.subscribe([&](auto&&... args)
        {
            onKey(args...);
        }));

    eventSubscriptions.push_back(window->mouseButtonEvent.subscribe([&](auto&&... args)
        {
            onMouseButton(args...);
        }));

    eventSubscriptions.push_back(window->cursorPosEvent.subscribe([&](auto&&... args)
        {
            onCursorPos(args...);
        }));

    eventSubscriptions.push_back(window->scrollEvent.subscribe([&](auto&&... args)
        {
            onScroll(args...);
        }));

    eventSubscriptions.push_back(window->cursorEnterEvent.subscribe([&](auto&&... args)
        {
            onCursorEnter(args...);
        }));
}

void InputManager::update()
{
    input->previous = input->current;
    input->current = next;

    next.mouseScroll = glm::vec2(0);
}

void InputManager::onKey(Window* window, int key, int scancode, int action, int mods)
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

void InputManager::onMouseButton(Window* window, int button, int action, int mods)
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

void InputManager::onCursorPos(Window* window, double xpos, double ypos)
{
    next.mousePosition.x = xpos;
    next.mousePosition.y = ypos;
}

void InputManager::onScroll(Window* window, double xoffset, double yoffset)
{
    next.mouseScroll.x = xoffset;
    next.mouseScroll.y = yoffset;
}

void InputManager::onCursorEnter(Window* window, int entered)
{
    cursorEnteredThisFrame = true;
}
