#pragma once

#include <windows.h>
#include <string>
#include <iostream>
#include "steam/steam_api.h"
#include "Game.h"
#include "Pipe.h"
#include "Request.h"
#include "Config.h"
#include "WorkshopUploadDialog.h"

class LauncherGUI {
public:
    LauncherGUI(HINSTANCE hInstance);
    ~LauncherGUI();
    bool Initialize();
    void Run();
    void LaunchGameWithLanguage();

private:
    HINSTANCE hInstance;
    HWND hComboBox;
    HWND hLaunchButton;
    HBITMAP hBackground;
    filesystem::path workingDirectory;
    bool InitializeSteam();
    void Cleanup();
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};