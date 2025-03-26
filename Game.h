#ifndef GAME_H
#define GAME_H

#include <windows.h>
#include <iostream>
#include <filesystem>

STARTUPINFO si;
PROCESS_INFORMATION pi;

bool LaunchGame(const char* gamePath) {
	if (!gamePath) {
		std::cerr << "Game executable is not defined\n";
		return false;
	}
	ZeroMemory(&si, sizeof(si));
	if (!CreateProcess(NULL, (LPSTR)gamePath, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
		std::cerr << "Failed to launch game executable\n";
		return false;
	}
	return true;
}

void CloseGame() {
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
}
#endif