#pragma once
#include "Character.h" // Character 데이터를 읽어와야 하므로 포함

class UIManager {
private:
    static void clearConsole();

public:
    static void renderConsoleUI(const Character& player);
};