#include <windows.h>
#include <shellapi.h>
#include <string>
#include <iostream>
#include <cstdlib>
#include <thread>
#include <atomic>
#include <chrono>

// 全局变量
NOTIFYICONDATA nid;
HWND hWnd;
HMENU hMenu;
HICON hIconGreen, hIconRed;
std::atomic<bool> wslRunning(false);
std::atomic<bool> programRunning(true);

// 函数声明
LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
bool IsWslRunning();
void ToggleWsl();
void UpdateIcon();
void MonitorWslStatus();
HICON CreateColoredIcon(COLORREF color);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // 注册窗口类
    const char CLASS_NAME[] = "WslMonitorClass";
    
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    
    RegisterClass(&wc);
    
    // 创建窗口
    hWnd = CreateWindowEx(
        0,                              // 扩展样式
        CLASS_NAME,                     // 窗口类名
        "WSL Monitor",                  // 窗口标题
        0,                              // 窗口样式
        CW_USEDEFAULT, CW_USEDEFAULT,   // 初始 x 和 y 坐标
        0, 0,                           // 初始宽度和高度
        NULL,                           // 父窗口句柄
        NULL,                           // 菜单句柄
        hInstance,                      // 实例句柄
        NULL                            // 创建参数
    );
    
    if (hWnd == NULL) {
        return 0;
    }
    
    // 创建图标
    hIconGreen = CreateColoredIcon(RGB(0, 255, 0)); // 绿色
    hIconRed = CreateColoredIcon(RGB(255, 0, 0));   // 红色
    
    // 设置托盘图标
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hWnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_USER + 1;
    nid.hIcon = hIconRed;
    wcscpy_s(nid.szTip,100, L"WSL 未运行");
    
    Shell_NotifyIcon(NIM_ADD, &nid);
    
    // 创建右键菜单
    hMenu = CreatePopupMenu();
    AppendMenu(hMenu, MF_STRING, 1, L"退出");
    
    // 启动WSL监控线程
    std::thread monitorThread(MonitorWslStatus);
    monitorThread.detach();
    
    // 消息循环
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    // 清理资源
    Shell_NotifyIcon(NIM_DELETE, &nid);
    DestroyIcon(hIconGreen);
    DestroyIcon(hIconRed);
    DestroyMenu(hMenu);
    
    return 0;
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY:
            programRunning = false;
            PostQuitMessage(0);
            return 0;
            
        case WM_USER + 1:
            switch (LOWORD(lParam)) {
                case WM_LBUTTONDOWN:
                    // 左键点击切换WSL状态
                    ToggleWsl();
                    return 0;
                    
                case WM_RBUTTONDOWN: {
                    // 右键点击显示菜单
                    POINT pt;
                    GetCursorPos(&pt);
                    
                    SetForegroundWindow(hWnd);
                    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
                    
                    return 0;
                }
            }
            break;
            
        case WM_COMMAND:
            if (LOWORD(wParam) == 1) {
                // 选择退出菜单
                programRunning = false;
                DestroyWindow(hWnd);
            }
            return 0;
    }
    
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

bool IsWslRunning() {
    FILE* pipe = _popen("wsl --list --running", "r");
    if (!pipe) return false;
    
    char buffer[128];
    std::string result = "";
    
    while (!feof(pipe)) {
        if (fgets(buffer, 128, pipe) != NULL)
            result += buffer;
    }
    
    _pclose(pipe);
    
    // 检查输出中是否包含除标题外的其他行
    size_t pos = result.find('\n');
    if (pos != std::string::npos && pos + 1 < result.length()) {
        std::string contentAfterFirstLine = result.substr(pos + 1);
        return !contentAfterFirstLine.empty() && contentAfterFirstLine.find_first_not_of(" \t\n\r") != std::string::npos;
    }
    
    return false;
}

void ToggleWsl() {
    if (wslRunning) {
        // 停止WSL
        std::system("wsl --shutdown");
        wslRunning = false;
        UpdateIcon();
    } else {
        // 启动WSL
        STARTUPINFO si = { sizeof(si) };
        PROCESS_INFORMATION pi;
        
        if (CreateProcess(NULL, (LPSTR)"wsl", NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            wslRunning = true;
            UpdateIcon();
        }
    }
}

void UpdateIcon() {
    nid.hIcon = wslRunning ? hIconGreen : hIconRed;
    wcscpy_s(nid.szTip, 100,wslRunning ? L"WSL 正在运行" : L"WSL 未运行");
    Shell_NotifyIcon(NIM_MODIFY, &nid);
}

void MonitorWslStatus() {
    while (programRunning) {
        bool currentStatus = IsWslRunning();
        if (currentStatus != wslRunning) {
            wslRunning = currentStatus;
            // 使用PostMessage确保在主线程中更新UI
            PostMessage(hWnd, WM_USER + 2, 0, 0);
        }
        // 修改检测间隔为5秒
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

HICON CreateColoredIcon(COLORREF color) {
    // 创建图标位图
    HBITMAP hbmColor = CreateBitmap(16, 16, 1, 32, NULL);
    HBITMAP hbmMask = CreateBitmap(16, 16, 1, 1, NULL);
    
    // 获取设备上下文
    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    
    // 选择位图
    HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmColor);
    
    // 绘制彩色部分
    HBRUSH hBrush = CreateSolidBrush(color);
    RECT rect = { 0, 0, 16, 16 };
    FillRect(hdcMem, &rect, hBrush);
    DeleteObject(hBrush);
    
    // 恢复设备上下文
    SelectObject(hdcMem, hbmOld);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
    
    // 创建图标
    ICONINFO ii = {};
    ii.fIcon = TRUE;
    ii.hbmColor = hbmColor;
    ii.hbmMask = hbmMask;
    
    HICON hIcon = CreateIconIndirect(&ii);
    
    // 清理资源
    DeleteObject(hbmColor);
    DeleteObject(hbmMask);
    
    return hIcon;
}