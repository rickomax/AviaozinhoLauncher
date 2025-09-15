#pragma once

#include <windows.h>
#include <iostream>
#include <string>
#include <filesystem>

STARTUPINFOA si{};
PROCESS_INFORMATION pi{};

bool LaunchGame(const std::string& gamePath)
{
    if (gamePath.empty()) {
        std::cerr << "Game executable is not defined\n";
        return false;
    }

    // Make a writable copy because CreateProcessA wants a modifiable char buffer.
    std::string cmdLine = gamePath;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    if (!CreateProcessA(
        nullptr,                      // application name (use command line instead)
        cmdLine.data(),               // modifiable command line buffer
        nullptr, nullptr,             // security attributes
        TRUE,                         // inherit handles
        0,                            // creation flags
        nullptr,                      // environment (inherit)
        CURRENT_DIRECTORY,            // starting directory
        &si,
        &pi))
    {
        std::cerr << "Failed to launch game executable, error: " << GetLastError() << "\n";
        return false;
    }

    return true;
}

void CloseGame()
{
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}
