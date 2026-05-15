#pragma once

#include <GLFW/glfw3.h>

class Input {
public:
    explicit Input(GLFWwindow* window = nullptr);

    void setWindow(GLFWwindow* newWindow);
    bool isKeyDown(int key) const;

private:
    GLFWwindow* window;
};
