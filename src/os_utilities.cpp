#include "../include/os_utilities.h"

#include <Windows.h>

void manta::createFolder(const char *folder) {
    if (CreateDirectory(folder, NULL) ||
        GetLastError() == ERROR_ALREADY_EXISTS) {
        // Folder was created successfully
    }
    else {
        // Failed to create folder
    }
}

void manta::showConsoleCursor(bool show) {
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);

    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(out, &cursorInfo);

    cursorInfo.bVisible = show;

    SetConsoleCursorInfo(out, &cursorInfo);
}

void manta::sleep(int milliseconds) {
    Sleep((DWORD)milliseconds);
}
