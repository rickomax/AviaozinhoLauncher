#ifndef LAUNCHER_GUI_H
#define LAUNCHER_GUI_H

#include <windows.h>
#include <string>
#include <iostream>
#include "steam/steam_api.h"
#include "Game.h"
#include "Pipe.h"
#include "Request.h"

class LauncherGUI {
public:
    LauncherGUI(HINSTANCE hInstance);
    ~LauncherGUI();
    bool Initialize();
    void Run();

private:
    HINSTANCE hInstance;
    HWND hwndMain;
    HWND hComboBox;
    HWND hLaunchButton;
    HBITMAP hBackground;
    bool InitializeSteamAndPipe();
    void Cleanup();
    void LaunchGameWithLanguage();
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

#endif // LAUNCHER_GUI_H