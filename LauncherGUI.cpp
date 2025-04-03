#include "LauncherGUI.h"

using namespace std;

#define ID_COMBOBOX 1001
#define ID_LAUNCH_BUTTON 1002

#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 640

#ifdef TESTES
int main(int argc, char* argv[]) {
	LauncherGUI launcher(GetModuleHandle(NULL));
	if (!launcher.Initialize()) {
		return 1;
	}
	launcher.Run();
	return 0;
}
#else
int main(int argc, char* argv[]) {
	HWND consoleWindow = GetConsoleWindow();
	if (consoleWindow != NULL) {
		ShowWindow(consoleWindow, SW_HIDE);
	}

	LauncherGUI launcher(GetModuleHandle(NULL));
	if (!launcher.Initialize()) {
		return 1;
	}
	launcher.Run();
	return 0;
}
#endif

LauncherGUI::LauncherGUI(HINSTANCE hInstance) : hInstance(hInstance), hwndMain(NULL), hComboBox(NULL), hLaunchButton(NULL), hBackground(NULL) {}

LauncherGUI::~LauncherGUI() {
	Cleanup();
}

bool LauncherGUI::Initialize() {
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
	if (SteamAPI_RestartAppIfNecessary(STEAM_APP_ID)) {
		cerr << "Restarting through Steam\n";
		return false;
	}

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
				"Opções",
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
				"Linguagem:",
				WS_CHILD | WS_VISIBLE | SS_LEFT,
				15,
				WINDOW_HEIGHT - 36,
				100,
				24,
				hwnd,
				NULL,
				pThis->hInstance,
				NULL
			);

			pThis->hComboBox = CreateWindow(
				"COMBOBOX",
				"",
				CBS_DROPDOWN | WS_CHILD | WS_VISIBLE,
				110,
				WINDOW_HEIGHT - 39,
				200,
				120,
				hwnd,
				(HMENU)ID_COMBOBOX,
				pThis->hInstance,
				NULL
			);
			SendMessage(pThis->hComboBox, CB_ADDSTRING, 0, (LPARAM)"Português");
			//SendMessage(pThis->hComboBox, CB_ADDSTRING, 0, (LPARAM)"English");
			SendMessage(pThis->hComboBox, CB_SETCURSEL, 0, 0);

			pThis->hLaunchButton = CreateWindow(
				"BUTTON",
				"Mandar Bala!",
				WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
				WINDOW_WIDTH - 117,
				WINDOW_HEIGHT - 43,
				100,
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
		//(index to get the dropdown index)
		string language_file = "loc_portuguese.txt";
		string executable_path = GAMEEXECUTABLE;
		string command_line = executable_path + " -language " + language_file;

		ShowWindow(hwndMain, SW_HIDE);

		if (!LaunchGame(command_line.c_str())) {
			cerr << "Error launching game\n";
			return;
		}

		if (!Pipe_ConnectToNew()) {
			cerr << "Error connecting pipe\n";
			return;
		}

#ifdef TESTES
		if (!SteamUserStats()->ResetAllStats(true)) {
			cerr << "Error resetting stats and achievements";
			return;
		}
#endif

		while (Pipe_Read()) {
			SteamAPI_RunCallbacks();
			ParseRequest();
		}

		PostQuitMessage(0);
	}
}