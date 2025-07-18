#include <windows.h>
#include <shellapi.h>
#include <string>
#include <cstdlib>

#define ID_TRAY_APP_ICON 1
#define ID_TRAY_EXIT 2
#define ID_TRAY_TOGGLE 3
#define WM_TRAYICON (WM_USER + 1)

NOTIFYICONDATA nid;
HICON hIconGreen, hIconRed;
HMENU hPopupMenu;
bool wslRunning = false;

bool CheckWSLRunning() {
    FILE* pipe = _popen("wsl --list --running", "r");
    if (!pipe) return false;
    
    char buffer[128];
    std::string result = "";
    while (!feof(pipe)) {
        if (fgets(buffer, 128, pipe) != NULL)
            result += buffer;
    }
    _pclose(pipe);
    return result.find("Ubuntu") != std::string::npos || 
           result.find("Debian") != std::string::npos;
}

void ToggleWSL() {
    if (wslRunning) {
        system("wsl --shutdown");
    } else {
        system("wsl -d Ubuntu");
    }
    wslRunning = !wslRunning;
    nid.hIcon = wslRunning ? hIconGreen : hIconRed;
    Shell_NotifyIcon(NIM_MODIFY, &nid);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
            hIconGreen = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(1));
            hIconRed = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(2));
            
            nid.cbSize = sizeof(NOTIFYICONDATA);
            nid.hWnd = hwnd;
            nid.uID = ID_TRAY_APP_ICON;
            nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
            nid.uCallbackMessage = WM_TRAYICON;
            nid.hIcon = hIconRed;
            strcpy(nid.szTip, "WSL Monitor");
            Shell_NotifyIcon(NIM_ADD, &nid);
            
            hPopupMenu = CreatePopupMenu();
            AppendMenu(hPopupMenu, MF_STRING, ID_TRAY_TOGGLE, "Toggle WSL");
            AppendMenu(hPopupMenu, MF_SEPARATOR, 0, NULL);
            AppendMenu(hPopupMenu, MF_STRING, ID_TRAY_EXIT, "Exit");
            break;
            
        case WM_TRAYICON:
            if (lParam == WM_RBUTTONUP) {
                POINT pt;
                GetCursorPos(&pt);
                SetForegroundWindow(hwnd);
                TrackPopupMenu(hPopupMenu, TPM_BOTTOMALIGN, pt.x, pt.y, 0, hwnd, NULL);
            } else if (lParam == WM_LBUTTONUP) {
                ToggleWSL();
            }
            break;
            
        case WM_COMMAND:
            if (LOWORD(wParam) == ID_TRAY_EXIT) {
                Shell_NotifyIcon(NIM_DELETE, &nid);
                PostQuitMessage(0);
            } else if (LOWORD(wParam) == ID_TRAY_TOGGLE) {
                ToggleWSL();
            }
            break;
            
        case WM_TIMER:
            wslRunning = CheckWSLRunning();
            nid.hIcon = wslRunning ? hIconGreen : hIconRed;
            Shell_NotifyIcon(NIM_MODIFY, &nid);
            break;
            
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "WSLTrayClass";
    RegisterClass(&wc);
    
    HWND hwnd = CreateWindow("WSLTrayClass", "WSL Monitor", 0, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);
    SetTimer(hwnd, 1, 5000, NULL); // 每5秒检查一次WSL状态
    
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return 0;
}
