#pragma once
#include <windows.h>
#include <string>

class WorkshopUploadDialog {
public:
    static int ShowModal(HWND owner, HINSTANCE hInstance);

private:
    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
    static void CenterOnOwner(HWND hwnd, HWND owner);
    static HFONT GetUIFont();

    static bool BrowseForPng(HWND owner, std::string& outPath);
    static bool BrowseForFolder(HWND owner, std::string& outPath);
};
