#pragma once

#include <GLFW/glfw3.h>

/// GLFW 입력 상태를 App 외부에서 읽을 수 있게 감싼 얇은 어댑터.
/// 현재는 키 눌림 상태만 제공하지만, 이후 mouse delta/pressed/released 상태 추적을 추가할 자리다.
class Input {
public:
    /// 입력을 읽을 GLFW window를 초기화한다.
    /// @param window GLFW 키 상태를 조회할 대상 윈도우. nullptr이면 모든 입력 조회는 false를 반환한다.
    explicit Input(GLFWwindow* window = nullptr);

    /// sb7 application이 window를 생성한 뒤 Input에 연결한다.
    /// @param newWindow 이후 isKeyDown에서 사용할 GLFW window 포인터.
    void setWindow(GLFWwindow* newWindow);

    /// 현재 프레임에 해당 키가 눌려 있는지 즉시 조회한다.
    /// @param key GLFW_KEY_* 상수.
    /// @return window가 유효하고 키가 GLFW_PRESS 상태이면 true.
    bool isKeyDown(int key) const;

private:
    /// GLFW 입력 API가 상태를 조회할 대상 창. 소유하지 않는 포인터다.
    GLFWwindow* window;
};
