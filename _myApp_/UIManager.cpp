#include "UIManager.h"
#include <windows.h> // cpp 파일에만 포함시킵니다!
#include <cstdio>    // printf용

void UIManager::clearConsole() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD coordScreen = { 0, 0 };
    DWORD cCharsWritten;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    DWORD dwConSize;

    if (!GetConsoleScreenBufferInfo(hConsole, &csbi)) return;
    dwConSize = csbi.dwSize.X * csbi.dwSize.Y;

    FillConsoleOutputCharacter(hConsole, (TCHAR)' ', dwConSize, coordScreen, &cCharsWritten);

    GetConsoleScreenBufferInfo(hConsole, &csbi);
    FillConsoleOutputAttribute(hConsole, csbi.wAttributes, dwConSize, coordScreen, &cCharsWritten);

    SetConsoleCursorPosition(hConsole, coordScreen);
}

void UIManager::renderConsoleUI(const Character& player) {
    clearConsole();

    printf("========================================\n");
    printf("        [ 에이메스 전투 시스템 UI ]      \n");
    printf("========================================\n");

    // %5.2f는 뭐지?
    printf(" [현재 위치] X: %5.2f | Y: %5.2f | Z: %5.2f \n", player.getX(), player.getY(), player.getZ());
    printf(" [상태] %s \n", player.getIsSprinting() ? ">> 달리는 중 <<" : "걷는 중");
    if (player.getIsDashing()) {
        printf(" [상태] >> 대쉬! << \n");
    }
    printf("----------------------------------------\n");

    printf(" 폼 상태: [%s] \n", player.getForm() == HUMAN ? "HUMAN" : "MECHA (엑소스트라이더)");
    printf(" 버프: %s \n", player.getIsEnhanced() ? "[강화 평타 장전됨!]" : "[없음]");
    printf("----------------------------------------\n");

    printf(" E 스택  : [");
    for (int i = 0; i < player.getMaxEStack(); i++) {
        if (i < player.getEStack()) printf("■");
        else printf("□");
    }
    printf("] (%d/%d)\n", player.getEStack(), player.getMaxEStack());

    printf(" 궁극기  : [");
    int gaugeBlocks = (int)(player.getUltGauge() / 10.0f);
    for (int i = 0; i < 10; i++) {
        if (i < gaugeBlocks) printf("■");
        else printf("□");
    }
    printf("] (%.0f%%)\n", player.getUltGauge());

    printf(" 2차 스택: [");
    for (int i = 0; i < player.getMaxUltStack(); i++) {
        if (i < player.getUltStack()) printf("■");
        else printf("□");
    }
    printf("] (%d/%d)\n", player.getUltStack(), player.getMaxUltStack());

    printf("========================================\n");
    printf(" 시스템 메시지: %s\n", player.getLastMessage().c_str());
}