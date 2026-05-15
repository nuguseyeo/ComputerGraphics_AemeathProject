#include "Input.h"

Input::Input(GLFWwindow* window)
    : window(window) {
}

void Input::setWindow(GLFWwindow* newWindow) {
    window = newWindow;
}

bool Input::isKeyDown(int key) const {
    return window && glfwGetKey(window, key) == GLFW_PRESS;
}
