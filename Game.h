#pragma once

#include <windows.h>
#include <iostream>
#include <string>
#include <filesystem>
#include "Static.h"

bool LaunchGame(const std::string& gamePath, PROCESS_INFORMATION* pi)
{
    if (gamePath.empty()) {
        ShowError("Game executable is not defined");
        return false;
    }
    std::string cmdLine = gamePath;
    STARTUPINFOA si{};
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    if (!CreateProcessA(
        nullptr,                      // application name (use command line instead)
        cmdLine.data(),               // modifiable command line buffer
        nullptr, nullptr,             // security attributes
        TRUE,                         // inherit handles
        0,                            // creation flags
        nullptr,                      // environment (inherit)
        nullptr,                      // starting directory
        &si,
        pi))
    {
        return false;
    }
    return true;
}

bool IsGameRunning(PROCESS_INFORMATION* pi)
{
    if (!pi->hProcess) {
        return false;
    }
    DWORD exitCode = 0;
    if (!GetExitCodeProcess(pi->hProcess, &exitCode))
    {
        return false;
    }
    return (exitCode == STILL_ACTIVE);
}