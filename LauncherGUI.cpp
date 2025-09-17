#include "LauncherGUI.h"
#include "WorkshopUploadDialog.h"

using namespace std;

#define ID_COMBOBOX         1001
#define ID_LAUNCH_BUTTON    1002
#define ID_UPLOAD_BUTTON    1003

#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 640

#ifdef CONSOLE_BUILD
int main(int argc, char* argv[]) {
    LauncherGUI launcher(GetModuleHandle(NULL));
    if (!launcher.Initialize()) {
        return 1;
    }
    launcher.Run();
    return 0;
}
#else
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    LauncherGUI launcher(hInstance);
    if (!launcher.Initialize()) return 1;
    launcher.Run();
    return 0;
}
#endif

LauncherGUI::LauncherGUI(HINSTANCE hInstance)
    : hInstance(hInstance), hwndMain(NULL), hComboBox(NULL), hLaunchButton(NULL), hBackground(NULL) {
}

LauncherGUI::~LauncherGUI() {
    Cleanup();
}

bool LauncherGUI::Initialize() {
    workingDirectory = filesystem::current_path();
    if (!InitializeSteamAndPipe()) {
        return false;
    }

    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "LauncherWindowClass";
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    if (!RegisterClass(&wc)) {
        cerr << "Error registering window class\n";
        return false;
    }

    RECT rect = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

    AdjustWindowRect(&rect, style, FALSE);
    int windowWidth = rect.right - rect.left;
    int windowHeight = rect.bottom - rect.top;

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int x = (screenWidth - windowWidth) / 2;
    int y = (screenHeight - windowHeight) / 2;

    hwndMain = CreateWindow(
        "LauncherWindowClass",
        "Aviãozinho do Tráfico 3",
        style,
        x, y, windowWidth, windowHeight,
        NULL, NULL, hInstance, this
    );
    if (!hwndMain) {
        cerr << "Error creating window\n";
        return false;
    }

    ShowWindow(hwndMain, SW_SHOW);
    UpdateWindow(hwndMain);

    return true;
}

bool LauncherGUI::InitializeSteamAndPipe() {
#ifndef STEAM_TEST
    if (SteamAPI_RestartAppIfNecessary(STEAM_APP_ID)) {
        cerr << "Restarting through Steam\n";
        return false;
    }
#endif

    if (!SteamAPI_Init()) {
        cerr << "Error initializing Steam\n";
        return false;
    }

    if (!Pipe_Create()) {
        cerr << "Error creating pipe\n";
        SteamAPI_Shutdown();
        return false;
    }

    return true;
}

void LauncherGUI::Cleanup() {
    Pipe_Close();
    SteamAPI_Shutdown();
}

void LauncherGUI::Run() {
    ShowWindow(hwndMain, SW_SHOW);
    UpdateWindow(hwndMain);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

LRESULT CALLBACK LauncherGUI::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    LauncherGUI* pThis = nullptr;
    if (msg == WM_CREATE) {
        CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        pThis = reinterpret_cast<LauncherGUI*>(pCreate->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
    }
    else {
        pThis = reinterpret_cast<LauncherGUI*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (pThis) {
        switch (msg) {
        case WM_CREATE:
        {
            CreateWindow(
                "BUTTON",
                "Opções (Options)",
                WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                0,
                WINDOW_HEIGHT - 65,
                WINDOW_WIDTH,
                65,
                hwnd,
                NULL,
                pThis->hInstance,
                NULL
            );

            CreateWindow(
                "STATIC",
                "Linguagem (Language):",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                15,
                WINDOW_HEIGHT - 36,
                200,
                24,
                hwnd,
                NULL,
                pThis->hInstance,
                NULL
            );

            pThis->hComboBox = CreateWindow(
                "COMBOBOX",
                "",
                CBS_DROPDOWNLIST | WS_VSCROLL | WS_CHILD | WS_VISIBLE,
                210,
                WINDOW_HEIGHT - 39,
                200,
                120,
                hwnd,
                (HMENU)ID_COMBOBOX,
                pThis->hInstance,
                NULL
            );

            auto languageMap = read_kv_pairs("languages.txt");
            for (auto pair : languageMap) {
                SendMessage(pThis->hComboBox, CB_ADDSTRING, 0, (LPARAM)pair.first.c_str());
            }
            DWORD selectedIndex;
            ReadDwordFromRegistry(selectedIndex, "language");
            if (selectedIndex >= 0 && selectedIndex < languageMap.size()) {
                SendMessage(pThis->hComboBox, CB_SETCURSEL, selectedIndex, 0);
            }

            // NEW: "Upload to Workshop" button (to the left of the launch button)
            CreateWindow(
                "BUTTON",
                "Workshop/Upload",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                WINDOW_WIDTH - 217 - 210,            // 200 width + 10px gap to the left of launch
                WINDOW_HEIGHT - 43,
                200,
                30,
                hwnd,
                (HMENU)ID_UPLOAD_BUTTON,
                pThis->hInstance,
                NULL
            );

            // Existing launch button
            pThis->hLaunchButton = CreateWindow(
                "BUTTON",
                "Mandar Bala! (Let it Rip!)",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                WINDOW_WIDTH - 217,
                WINDOW_HEIGHT - 43,
                200,
                30,
                hwnd,
                (HMENU)ID_LAUNCH_BUTTON,
                pThis->hInstance,
                NULL
            );

            pThis->hBackground = (HBITMAP)LoadImage(NULL, "background.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
            if (!pThis->hBackground) {
                cerr << "Error loading background image\n";
            }
            break;
        }
        case WM_COMMAND:
            if (LOWORD(wParam) == ID_LAUNCH_BUTTON) {
                pThis->LaunchGameWithLanguage();
            }
            else if (LOWORD(wParam) == ID_UPLOAD_BUTTON) {
                // Open modal upload window
                WorkshopUploadDialog::ShowModal(hwnd, pThis->hInstance);
            }
            break;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            if (pThis->hBackground) {
                HDC hdcMem = CreateCompatibleDC(hdc);
                SelectObject(hdcMem, pThis->hBackground);
                BitBlt(hdc, 0, 0, 1024, 768, hdcMem, 0, 0, SRCCOPY);
                DeleteDC(hdcMem);
            }
            EndPaint(hwnd, &ps);
            break;
        }
        case WM_DESTROY:
            if (pThis->hBackground) {
                DeleteObject(pThis->hBackground);
            }
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void LauncherGUI::LaunchGameWithLanguage() {
    int index = SendMessage(hComboBox, CB_GETCURSEL, 0, 0);
    if (index != CB_ERR) {
        filesystem::current_path(workingDirectory);

#ifdef CONSOLE_BUILD
        cout << "Working Directory:" << workingDirectory << endl;
        cout << "Game Executable:" << GAMEEXECUTABLE << endl;
        if (CURRENT_DIRECTORY) {
            cout << "Content Directory:" << CURRENT_DIRECTORY << endl;
        }
#endif

        WriteDwordToRegistry(index, "language");

        auto languageMap = read_kv_pairs("languages.txt");
        string language_file = languageMap[index].second;

        string executable_path = GAMEEXECUTABLE;
        string command_line = executable_path + " -language " + language_file;

        ShowWindow(hwndMain, SW_HIDE);

        if (!LaunchGame(command_line)) {
            cerr << "Error launching game\n";
            return;
        }

        if (!Pipe_ConnectToNew()) {
            cerr << "Error connecting pipe\n";
            return;
        }

        while (Pipe_Read()) {
            SteamAPI_RunCallbacks();
            ParseRequest(hwndMain);
        }

        PostQuitMessage(0);
    }
}
