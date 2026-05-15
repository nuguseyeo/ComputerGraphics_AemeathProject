#pragma once
#include "Character.h" // Character 데이터를 읽어와야 하므로 포함

// UIManager는 Windows 콘솔에 표시되는 디버그/전투 UI만 담당합니다.
// OpenGL 렌더링과 분리되어 있으며 Character의 읽기 전용 getter만 사용합니다.
class UIManager {
private:
    // 콘솔 버퍼를 공백으로 채우고 커서를 좌상단으로 되돌립니다.
    // 매 프레임 출력이 누적되지 않도록 renderConsoleUI 시작 시 호출됩니다.
    static void clearConsole();

public:
    // player는 화면에 표시할 현재 캐릭터 상태입니다.
    // 위치, 폼, 스킬 스택, 궁극기 게이지, 마지막 액션 메시지를 콘솔에 출력합니다.
    static void renderConsoleUI(const Character& player);
};