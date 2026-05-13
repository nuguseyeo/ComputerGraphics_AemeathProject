#include "UIManager.h"
#include <windows.h>
#include <cstdio>

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
    printf("        [ Aemeath Combat UI ]           \n");
    printf("========================================\n");

    printf(" [Position] X: %5.2f | Y: %5.2f | Z: %5.2f \n", player.getX(), player.getY(), player.getZ());
    printf(" [Move] %s \n", player.getIsSprinting() ? ">> Running <<" : "Walking");
    if (player.getIsDashing()) {
        printf(" [Move] >> Dash << \n");
    }
    printf("----------------------------------------\n");

    printf(" Form: [%s] \n", player.getForm() == HUMAN ? "HUMAN" : "MECHA");
    printf(" Buff: %s \n", player.getIsEnhanced() ? "[Enhanced attack ready]" : "[None]");
    printf("----------------------------------------\n");

    printf(" E Stack : [");
    for (int i = 0; i < player.getMaxEStack(); i++) {
        printf(i < player.getEStack() ? "#" : "-");
    }
    printf("] (%d/%d)\n", player.getEStack(), player.getMaxEStack());

    printf(" Ultimate: [");
    int gaugeBlocks = (int)(player.getUltGauge() / 10.0f);
    for (int i = 0; i < 10; i++) {
        printf(i < gaugeBlocks ? "#" : "-");
    }
    printf("] (%.0f%%)\n", player.getUltGauge());

    printf(" UltStack: [");
    for (int i = 0; i < player.getMaxUltStack(); i++) {
        printf(i < player.getUltStack() ? "#" : "-");
    }
    printf("] (%d/%d)\n", player.getUltStack(), player.getMaxUltStack());

    printf("========================================\n");
    printf(" Message: %s\n", player.getLastMessage().c_str());
}
