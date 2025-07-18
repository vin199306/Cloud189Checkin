
#include <windows.h>
#include <shellapi.h>
#include <string>
#include <cstdlib>

NOTIFYICONDATA nid;
HWND hwnd;
bool wslRunning = false;

// 检测WSL运行状态
bool checkWSLStatus() {
    return system("wsl -l --running > nul 2>&1") == 0;
}

// 更新任务栏图标
void updateIcon() {
    nid.hIcon = LoadIcon(NULL, wslRunning ? IDI_APPLICATION : IDI_ERROR);
    strcpy(nid.szTip, wslRunning ? "WSL正在运行" : "WSL空闲中");
    Shell_NotifyIcon(NIM_MODIFY, &nid);
}

// 切换WSL状态
void toggleWSL() {
    if(wslRunning) {
        system("wsl --shutdown");
    } else {
        system("wsl");
    }
    wslRunning = checkWSLStatus();
    updateIcon();
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch(msg) {
        case WM_CREATE:
            nid = { sizeof(nid) };
            nid.hWnd = hwnd;
            nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
            nid.uCallbackMessage = WM_APP;
            strcpy(nid.szTip, "WSL状态监控");
            updateIcon();
            Shell_NotifyIcon(NIM_ADD, &nid);
            break;
            
        case WM_APP:
            if(lParam == WM_LBUTTONUP) {
                toggleWSL();
            }
            break;
            
        case WM_DESTROY:
            Shell_NotifyIcon(NIM_DELETE, &nid);
            PostQuitMessage(0);
            break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    wslRunning = checkWSLStatus();
    
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "WSLMonitor";
    RegisterClass(&wc);
    
    hwnd = CreateWindow("WSLMonitor", NULL, 0, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);
    
    MSG msg;
    while(GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
