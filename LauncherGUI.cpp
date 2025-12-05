#include "LauncherGUI.h"

using namespace std;

#define ID_COMBOBOX         1001
#define ID_LAUNCH_BUTTON    1002
#define ID_UPLOAD_BUTTON    1003

#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 640

static std::string Utf16ToUtf8(PWSTR ws)
{
	if (!ws) return {};

	int size_needed = WideCharToMultiByte(
		CP_UTF8, 0, ws, -1, nullptr, 0, nullptr, nullptr
	);

	if (size_needed <= 0)
		return {};

	std::string result(size_needed - 1, 0);

	WideCharToMultiByte(
		CP_UTF8, 0, ws, -1, result.data(), size_needed, nullptr, nullptr
	);

	return result;
}

#ifdef CONSOLE_BUILD
int main(int argc, char* argv[]) {
	if (argc > 1) {
		commandLine = std::string(argv[1], strlen(argv[1]));
	}
	LauncherGUI launcher(GetModuleHandle(NULL));
	if (!launcher.Initialize()) {
		return 1;
	}
	launcher.Run();
	return 0;
}
#else
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
	commandLine = Utf16ToUtf8(pCmdLine);
	LauncherGUI launcher(hInstance);
	if (!launcher.Initialize()) {
		return 1;
	}
	launcher.Run();
	return 0;
}
#endif

LauncherGUI::LauncherGUI(HINSTANCE hInstance)
	: hInstance(hInstance), hComboBox(NULL), hLaunchButton(NULL), hBackground(NULL) {
}

LauncherGUI::~LauncherGUI() {
	Cleanup();
}

bool LauncherGUI::Initialize() {
	workingDirectory = filesystem::current_path();
	if (!InitializeSteam()) {
		return false;
	}

	WNDCLASS wc = { 0 };
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = "LauncherWindowClass";
	wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	if (!RegisterClass(&wc)) {
		ShowError("Error registering window class");
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

	mainHwnd = CreateWindow(
		"LauncherWindowClass",
		"Aviãozinho do Tráfico 3",
		style,
		x, y, windowWidth, windowHeight,
		NULL, NULL, hInstance, this
	);
	if (!mainHwnd) {
		ShowError("Error creating window");
		return false;
	}
	UpdateWindow(mainHwnd);
	return true;
}

bool LauncherGUI::InitializeSteam() {
#ifndef STEAM_TEST
	if (SteamAPI_RestartAppIfNecessary(STEAM_APP_ID)) {
		ShowError("Restarting through Steam");
		return false;
	}
#endif
	if (!SteamAPI_Init()) {
		ShowError("Error initializing Steam");
		return false;
	}
	return true;
}

void LauncherGUI::Cleanup() {
	Pipe_Close();
	NetPipe_Close();
	SteamAPI_Shutdown();
}

void LauncherGUI::Run() {
#ifdef NO_LAUNCHER
	LaunchGameWithLanguage();
#else
	if (!SteamUtils()->IsSteamRunningOnSteamDeck()) {
		ShowWindow(mainHwnd, SW_SHOW);
	}
	else {
		LaunchGameWithLanguage();
	}
#endif
	UpdateWindow(mainHwnd);
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		SteamAPI_RunCallbacks();
		if (!commandLine.empty()) {
			LaunchGameWithLanguage();
		}
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
			languageMap = read_kv_pairs("languages.txt");
			for (auto pair : languageMap) {
				SendMessage(pThis->hComboBox, CB_ADDSTRING, 0, (LPARAM)pair.first.c_str());
			}
			settings = read_kv_pairs("settings.txt");
			string selectedIndexText = find_value_or_default(settings, "language", "0");
			DWORD selectedIndex = atoi(selectedIndexText.c_str());
			if (selectedIndex >= 0 && selectedIndex < languageMap.size()) {
				SendMessage(pThis->hComboBox, CB_SETCURSEL, selectedIndex, 0);
			}
			CreateWindow(
				"BUTTON",
				"Workshop/Upload",
				WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
				WINDOW_WIDTH - 217 - 210,
				WINDOW_HEIGHT - 43,
				200,
				30,
				hwnd,
				(HMENU)ID_UPLOAD_BUTTON,
				pThis->hInstance,
				NULL
			);
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
				ShowError("Error loading background image");
			}
			break;
		}
		case WM_COMMAND:
			if (LOWORD(wParam) == ID_LAUNCH_BUTTON) {
				pThis->LaunchGameWithLanguage();
			}
			else if (LOWORD(wParam) == ID_UPLOAD_BUTTON) {
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
	relaunch:
	int index = SendMessage(hComboBox, CB_GETCURSEL, 0, 0);
	if (index != CB_ERR) {
		filesystem::current_path(workingDirectory);

		vector<pair<string, string>> settings{
			{"language", to_string(index)}
		};
		write_kv_pairs("settings.txt", settings);

		string language_file = languageMap[index].second;
		string executable_path = GAMEEXECUTABLE;
		string command_line = executable_path + " -language " + language_file + " " + commandLine;
		commandLine.clear();

		if (!Pipe_Create()) {
			ShowError("Error creating main pipe");
			goto end;
		}
		Pipe_BeginConnect();

		if (!NetPipe_Create()) {
			ShowError("Error creating network pipe");
			goto end;
		}
		NetPipe_BeginConnect();

		PROCESS_INFORMATION pi;
		if (!LaunchGame(command_line, &pi)) {
			ShowError("Error launching game");
			goto end;
		}

		ShowWindow(mainHwnd, SW_HIDE);

		while (IsGameRunning(&pi)) {
			SteamAPI_RunCallbacks();
			if (Pipe_IsConnected()) {
				PumpPipe();
			}
			if (NetPipe_IsConnected()) {
				gns_pumppipe();
			}
			if (!commandLine.empty()) {
				TerminateProcess(pi.hProcess, EXIT_FAILURE);
				goto relaunch;
			}
		}
	}

end:
	SteamAPI_Shutdown();
	PostQuitMessage(0);
	return;
}

