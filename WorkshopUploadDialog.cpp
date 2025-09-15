#include "WorkshopUploadDialog.h"
#include "Uploader.h" 
#include "steam/steam_api.h"

#include <commdlg.h> 
#include <shlobj.h>
#include <shellapi.h>
#include <cassert>
#include <memory>

#define ID_UP_TITLE_LABEL    1100
#define ID_UP_TITLE_EDIT     1101
#define ID_UP_DESC_LABEL     1102
#define ID_UP_DESC_EDIT      1103
#define ID_UP_PREV_LABEL     1104
#define ID_UP_PREV_EDIT      1105
#define ID_UP_PREV_BROWSE    1106
#define ID_UP_CONT_LABEL     1107
#define ID_UP_CONT_EDIT      1108
#define ID_UP_CONT_BROWSE    1109
#define ID_UP_OK             1110
#define ID_UP_CANCEL         1111
#define ID_UP_STATUS         1112

static const int DLG_W = 640;
static const int DLG_H = 340;
static const UINT_PTR TIMER_ID_STEAM = 9001;
static const UINT     WM_UPLOAD_DONE = WM_APP + 42;

struct UploadState {
    HINSTANCE hInst{};
    HWND hTitle{}, hDesc{}, hPrev{}, hCont{}, hOk{}, hCancel{}, hStatus{};
    std::unique_ptr<Uploader> uploader;
    bool uploading = false;
};

int WorkshopUploadDialog::ShowModal(HWND owner, HINSTANCE hInst)
{
    static bool s_registered = false;
    if (!s_registered) {
        WNDCLASS wc{};
        wc.lpfnWndProc = WndProc;
        wc.hInstance = hInst;
        wc.lpszClassName = "UploadDialogClass";
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        s_registered = RegisterClass(&wc) != 0;
    }

    HWND hDlg = CreateWindowEx(
        WS_EX_DLGMODALFRAME,
        "UploadDialogClass",
        "Upload Workshop Content",
        WS_CAPTION | WS_SYSMENU | WS_POPUP | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, DLG_W, DLG_H,
        owner, NULL, hInst, NULL);

    if (!hDlg) return IDCANCEL;

    EnableWindow(owner, FALSE);
    CenterOnOwner(hDlg, owner);

    MSG msg;
    int result = IDCANCEL;
    BOOL running = TRUE;
    while (running && GetMessage(&msg, NULL, 0, 0)) {
        if (msg.message == WM_UPLOAD_DONE) {
            result = (int)msg.wParam;
            running = FALSE;
            break;
        }
        if (!IsDialogMessage(hDlg, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    EnableWindow(owner, TRUE);
    SetForegroundWindow(owner);
    return result;
}

LRESULT CALLBACK WorkshopUploadDialog::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    UploadState* st = reinterpret_cast<UploadState*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    switch (msg)
    {
    case WM_CREATE:
    {
        auto* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        auto* state = new UploadState{};
        state->hInst = (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)state);

        HFONT f = GetUIFont();

        const int L = 16, T = 16, W = 480, H = 24, GAP = 10;
        int y = T;

        // Title
        CreateWindow("STATIC", "Title:", WS_CHILD | WS_VISIBLE, L, y + 4, 110, 20, hwnd, (HMENU)ID_UP_TITLE_LABEL, NULL, NULL);
        state->hTitle = CreateWindow("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP,
            L + 110, y, W, H, hwnd, (HMENU)ID_UP_TITLE_EDIT, NULL, NULL);
        SendMessage(state->hTitle, WM_SETFONT, (WPARAM)f, TRUE);
        SendMessage(state->hTitle, EM_SETLIMITTEXT, 300, 0);
        y += H + GAP;

        // Description
        CreateWindow("STATIC", "Description:", WS_CHILD | WS_VISIBLE, L, y + 4, 110, 20, hwnd, (HMENU)ID_UP_DESC_LABEL, NULL, NULL);
        state->hDesc = CreateWindow("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOVSCROLL | ES_MULTILINE | ES_WANTRETURN | WS_TABSTOP,
            L + 110, y, W, 80, hwnd, (HMENU)ID_UP_DESC_EDIT, NULL, NULL);
        SendMessage(state->hDesc, WM_SETFONT, (WPARAM)f, TRUE);
        SendMessage(state->hDesc, EM_SETLIMITTEXT, 300, 0);
        y += 80 + GAP;

        // Preview image
        CreateWindow("STATIC", "Preview image (.png):", WS_CHILD | WS_VISIBLE, L, y + 4, 110, 20, hwnd, (HMENU)ID_UP_PREV_LABEL, NULL, NULL);
        state->hPrev = CreateWindow("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_READONLY | WS_TABSTOP,
            L + 110, y, W - 90, H, hwnd, (HMENU)ID_UP_PREV_EDIT, NULL, NULL);
        HWND hPrevBtn = CreateWindow("BUTTON", "Browse...", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
            L + 110 + (W - 90) + 8, y, 80, H, hwnd, (HMENU)ID_UP_PREV_BROWSE, NULL, NULL);
        SendMessage(state->hPrev, WM_SETFONT, (WPARAM)f, TRUE);
        SendMessage(hPrevBtn, WM_SETFONT, (WPARAM)f, TRUE);
        y += H + GAP;

        // Content folder
        CreateWindow("STATIC", "Content folder:", WS_CHILD | WS_VISIBLE, L, y + 4, 110, 20, hwnd, (HMENU)ID_UP_CONT_LABEL, NULL, NULL);
        state->hCont = CreateWindow("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_READONLY | WS_TABSTOP,
            L + 110, y, W - 90, H, hwnd, (HMENU)ID_UP_CONT_EDIT, NULL, NULL);
        HWND hContBtn = CreateWindow("BUTTON", "Browse...", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
            L + 110 + (W - 90) + 8, y, 80, H, hwnd, (HMENU)ID_UP_CONT_BROWSE, NULL, NULL);
        SendMessage(state->hCont, WM_SETFONT, (WPARAM)f, TRUE);
        SendMessage(hContBtn, WM_SETFONT, (WPARAM)f, TRUE);
        y += H + GAP + 6;

        // Status
        state->hStatus = CreateWindow("STATIC", "Idle", WS_CHILD | WS_VISIBLE | SS_LEFT,
            L, DLG_H - 40 - 16, 300, 20, hwnd, (HMENU)ID_UP_STATUS, NULL, NULL);
        SendMessage(state->hStatus, WM_SETFONT, (WPARAM)f, TRUE);

        // Buttons
        state->hOk = CreateWindow("BUTTON", "Upload", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP,
            DLG_W - 200 - 24, DLG_H - 70 - 16, 90, 28, hwnd, (HMENU)ID_UP_OK, NULL, NULL);
        state->hCancel = CreateWindow("BUTTON", "Cancel", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
            DLG_W - 100 - 24, DLG_H - 70 - 16, 90, 28, hwnd, (HMENU)ID_UP_CANCEL, NULL, NULL);
        SendMessage(state->hOk, WM_SETFONT, (WPARAM)f, TRUE);
        SendMessage(state->hCancel, WM_SETFONT, (WPARAM)f, TRUE);

        // Set fonts for labels too
        for (int id : { ID_UP_TITLE_LABEL, ID_UP_DESC_LABEL, ID_UP_PREV_LABEL, ID_UP_CONT_LABEL }) {
            HWND h = GetDlgItem(hwnd, id);
            if (h) SendMessage(h, WM_SETFONT, (WPARAM)f, TRUE);
        }

        SetFocus(state->hTitle);
        return 0;
    }

    case WM_COMMAND:
        if (!st) break;
        switch (LOWORD(wParam)) {
        case ID_UP_PREV_BROWSE: {
            std::string path;
            if (BrowseForPng(hwnd, path)) {
                SetWindowText(st->hPrev, path.c_str());
            }
        } break;

        case ID_UP_CONT_BROWSE: {
            std::string path;
            if (BrowseForFolder(hwnd, path)) {
                SetWindowText(st->hCont, path.c_str());
            }
        } break;

        case ID_UP_OK: {
            if (st->uploading) return 0;

            // Read inputs
            char titleBuf[64]{}, descBuf[256]{}, prevBuf[MAX_PATH]{}, contBuf[MAX_PATH]{};
            GetWindowText(st->hTitle, titleBuf, (int)sizeof(titleBuf));
            GetWindowText(st->hDesc, descBuf, (int)sizeof(descBuf));
            GetWindowText(st->hPrev, prevBuf, (int)sizeof(prevBuf));
            GetWindowText(st->hCont, contBuf, (int)sizeof(contBuf));
            std::string title = titleBuf, desc = descBuf, preview = prevBuf, content = contBuf;

            // Basic validation
            if (title.empty() || title.size() > 20) { MessageBox(hwnd, "Title is required.", "Validation", MB_ICONWARNING); return 0; }
            if (preview.empty()) { MessageBox(hwnd, "Select a PNG preview image.", "Validation", MB_ICONWARNING); return 0; }
            if (content.empty()) { MessageBox(hwnd, "Select a content folder.", "Validation", MB_ICONWARNING); return 0; }

            // Disable UI while uploading
            EnableWindow(st->hOk, FALSE);
            EnableWindow(st->hCancel, FALSE);
            SetWindowText(st->hStatus, "Uploading...");

            // Start uploader (non-blocking, assumes SteamAPI already initialized by the launcher)
            std::vector<std::string> tags; // no tags UI for now
            auto visibility = k_ERemoteStoragePublishedFileVisibilityPublic;

            st->uploader = std::make_unique<Uploader>(
                (AppId_t)STEAM_APP_ID, content, preview, title, desc, tags, visibility, k_PublishedFileIdInvalid);

            if (!st->uploader->Start(/*manageSteamLifecycle=*/false)) {
                MessageBox(hwnd, "Failed to start upload.", "Uploader", MB_ICONERROR);
                EnableWindow(st->hOk, TRUE);
                EnableWindow(st->hCancel, TRUE);
                SetWindowText(st->hStatus, "Idle");
                st->uploader.reset();
                return 0;
            }

            st->uploading = true;
            SetTimer(hwnd, TIMER_ID_STEAM, 15, nullptr); // pump callbacks
        } break;

        case ID_UP_CANCEL:
            if (!st->uploading) {
                // Close dialog
                PostMessage(GetParent(hwnd), WM_UPLOAD_DONE, IDCANCEL, 0);
                DestroyWindow(hwnd);
            }
            // If uploading, ignore cancel (or implement cancellation here)
            return 0;
        }
        return 0;

    case WM_TIMER:
        if (wParam == TIMER_ID_STEAM && st && st->uploading && st->uploader) {
            st->uploader->PumpOnce(); // SteamAPI_RunCallbacks()

            if (st->uploader->IsDone()) {
                KillTimer(hwnd, TIMER_ID_STEAM);
                st->uploading = false;

                if (st->uploader->IsSuccess()) {
                    char msg[256];
                    snprintf(msg, sizeof(msg),
                        "Upload successful.\nPublishedFileId: %llu%s",
                        (unsigned long long)st->uploader->PublishedId(),
                        st->uploader->NeedsLegalAgreement() ? "\n(You need to accept the Steam Workshop legal agreement.)" : "");
                    MessageBox(hwnd, msg, "Uploader", MB_ICONINFORMATION);
                    PostMessage(GetParent(hwnd), WM_UPLOAD_DONE, IDOK, 0);
                    DestroyWindow(hwnd);
                }
                else {
                    std::string err = st->uploader->LastError();
                    if (err.empty()) err = "Upload failed.";
                    MessageBox(hwnd, err.c_str(), "Uploader", MB_ICONERROR);
                    // Re-enable to allow retry
                    EnableWindow(st->hOk, TRUE);
                    EnableWindow(st->hCancel, TRUE);
                    SetWindowText(st->hStatus, "Idle");
                    st->uploader.reset();
                }
            }
        }
        return 0;

    case WM_CLOSE:
        if (st && st->uploading) return 0; // block close during upload
        PostMessage(GetParent(hwnd), WM_UPLOAD_DONE, IDCANCEL, 0);
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY: {
        auto* state = st;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
        delete state;
        return 0;
    }
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void WorkshopUploadDialog::CenterOnOwner(HWND hwnd, HWND owner)
{
    RECT rcDlg{}, rcOwner{};
    GetWindowRect(hwnd, &rcDlg);
    GetWindowRect(owner, &rcOwner);
    int w = rcDlg.right - rcDlg.left;
    int h = rcDlg.bottom - rcDlg.top;
    int x = rcOwner.left + ((rcOwner.right - rcOwner.left) - w) / 2;
    int y = rcOwner.top + ((rcOwner.bottom - rcOwner.top) - h) / 2;
    SetWindowPos(hwnd, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

HFONT WorkshopUploadDialog::GetUIFont()
{
    return (HFONT)GetStockObject(DEFAULT_GUI_FONT);
}

bool WorkshopUploadDialog::BrowseForPng(HWND owner, std::string& outPath)
{
    char file[MAX_PATH] = { 0 };
    OPENFILENAME ofn{}; ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = "PNG files (*.png)\0*.png\0All files (*.*)\0*.*\0\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (GetOpenFileName(&ofn)) {
        outPath = file;
        return true;
    }
    return false;
}

// Helper: convert wide path to ANSI (ACP) std::string to match the rest of the codebase
static std::string WideToAnsi(const std::wstring& w)
{
    if (w.empty()) return {};
    int len = WideCharToMultiByte(CP_ACP, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (len <= 1) return {};
    std::string s(static_cast<size_t>(len - 1), '\0');
    WideCharToMultiByte(CP_ACP, 0, w.c_str(), -1, s.data(), len, nullptr, nullptr);
    return s;
}

bool WorkshopUploadDialog::BrowseForFolder(HWND owner, std::string& outPath)
{
    // Initialize COM for this thread if needed.
    bool comInit = false;
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (SUCCEEDED(hr)) comInit = true; // S_OK or S_FALSE both mean it's okay to CoUninitialize later

    IFileDialog* pfd = nullptr;
    hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd));
    if (FAILED(hr)) {
        if (comInit) CoUninitialize();
        return false;
    }

    // Pick folders with filesystem paths (no virtual items), and don't change our process CWD.
    DWORD opts = 0;
    if (SUCCEEDED(pfd->GetOptions(&opts))) {
        opts |= FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_NOCHANGEDIR;
        pfd->SetOptions(opts);
    }

    pfd->SetTitle(L"Select content folder");

    hr = pfd->Show(owner);
    if (FAILED(hr)) {
        pfd->Release();
        if (comInit) CoUninitialize();
        return false; // cancelled or error
    }

    IShellItem* psi = nullptr;
    hr = pfd->GetResult(&psi);
    if (FAILED(hr) || !psi) {
        if (psi) psi->Release();
        pfd->Release();
        if (comInit) CoUninitialize();
        return false;
    }

    PWSTR pszPath = nullptr;
    hr = psi->GetDisplayName(SIGDN_FILESYSPATH, &pszPath);
    bool ok = SUCCEEDED(hr) && pszPath && *pszPath;

    if (ok) {
        outPath = WideToAnsi(pszPath);
    }

    if (pszPath) CoTaskMemFree(pszPath);
    psi->Release();
    pfd->Release();
    if (comInit) CoUninitialize();

    return ok;
}