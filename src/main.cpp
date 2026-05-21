#include <windows.h>
#include <shellapi.h>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shell32.lib")

#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_OPEN 1001
#define ID_TRAY_EXIT 1002
#define ID_FILE_EXIT 2001

HINSTANCE g_hInst = NULL;
HWND g_hMainWnd = NULL;
HWND g_hHiddenWnd = NULL;
NOTIFYICONDATAW g_nid = {};
BOOL g_bWindowVisible = FALSE;
HANDLE g_hMutex = NULL;

const wchar_t MUTEX_NAME[] = L"Global\\TrayApp_Single_Instance";

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK HiddenWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void AddTrayIcon(HWND hWnd);
void RemoveTrayIcon();
void ShowMainWindow();
void HideMainWindow();
void CreateMainWindow();
HWND CreateHiddenWindow();
void ShowContextMenu(HWND hWnd);
void HandleTrayMessage(WPARAM wParam, LPARAM lParam);
BOOL CheckSingleInstance();
void ReleaseSingleInstance();

BOOL CheckSingleInstance() {
    g_hMutex = CreateMutexW(NULL, TRUE, MUTEX_NAME);
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        if (g_hMutex) CloseHandle(g_hMutex);
        return FALSE;
    }
    return TRUE;
}

void ReleaseSingleInstance() {
    if (g_hMutex) {
        ReleaseMutex(g_hMutex);
        CloseHandle(g_hMutex);
        g_hMutex = NULL;
    }
}

void AddTrayIcon(HWND hWnd) {
    memset(&g_nid, 0, sizeof(NOTIFYICONDATAW));
    g_nid.cbSize = sizeof(NOTIFYICONDATAW);
    g_nid.hWnd = hWnd;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcscpy_s(g_nid.szTip, 128, L"Tray Application");
    Shell_NotifyIconW(NIM_ADD, &g_nid);
}

void RemoveTrayIcon() {
    if (g_nid.hWnd) {
        Shell_NotifyIconW(NIM_DELETE, &g_nid);
        g_nid.hWnd = NULL;
    }
}

void ShowMainWindow() {
    if (g_hMainWnd) {
        ShowWindow(g_hMainWnd, SW_SHOW);
        SetForegroundWindow(g_hMainWnd);
        g_bWindowVisible = TRUE;
    }
}

void HideMainWindow() {
    if (g_hMainWnd) {
        ShowWindow(g_hMainWnd, SW_HIDE);
        g_bWindowVisible = FALSE;
    }
}

void ShowContextMenu(HWND hWnd) {
    HMENU hMenu = CreatePopupMenu();
    InsertMenuW(hMenu, 0, MF_BYPOSITION | MF_STRING, ID_TRAY_OPEN, L"\u041E\u0442\u043A\u0440\u044B\u0442\u044C");
    InsertMenuW(hMenu, 1, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
    InsertMenuW(hMenu, 2, MF_BYPOSITION | MF_STRING, ID_TRAY_EXIT, L"\u0412\u044B\u0445\u043E\u0434");
    
    POINT pt;
    GetCursorPos(&pt);
    SetForegroundWindow(hWnd);
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
    PostMessageW(hWnd, WM_NULL, 0, 0);
    DestroyMenu(hMenu);
}

void HandleTrayMessage(WPARAM wParam, LPARAM lParam) {
    if (wParam != 1) return;
    
    switch (lParam) {
        case WM_LBUTTONDOWN:
            if (g_bWindowVisible) HideMainWindow();
            else ShowMainWindow();
            break;
        case WM_RBUTTONDOWN:
            ShowContextMenu(g_hHiddenWnd);
            break;
    }
}

void CreateMainWindow() {
    const wchar_t CLASS_NAME[] = L"TrayAppMainWindow";
    
    WNDCLASSW wc = {};
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = g_hInst;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassW(&wc);
    
    g_hMainWnd = CreateWindowExW(0, CLASS_NAME, L"Tray Application",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 500, 400,
        NULL, NULL, g_hInst, NULL);
    
    HMENU hMenuBar = CreateMenu();
    HMENU hFileMenu = CreatePopupMenu();
    AppendMenuW(hFileMenu, MF_STRING, ID_FILE_EXIT, L"\u0412\u044B\u0445\u043E\u0434");
    AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hFileMenu, L"\u0424\u0430\u0439\u043B");
    SetMenu(g_hMainWnd, hMenuBar);
    
    CreateWindowW(L"STATIC",
        L"\u041F\u0440\u0438\u043B\u043E\u0436\u0435\u043D\u0438\u0435 \u0440\u0430\u0431\u043E\u0442\u0430\u0435\u0442 \u0432 \u0441\u0438\u0441\u0442\u0435\u043C\u043D\u043E\u043C \u0442\u0440\u0435\u0435\n\n"
        L"\u2022 \u041B\u0435\u0432\u0430\u044F \u043A\u043D\u043E\u043F\u043A\u0430 \u043D\u0430 \u0438\u043A\u043E\u043D\u043A\u0435 - \u043F\u043E\u043A\u0430\u0437\u0430\u0442\u044C/\u0441\u043A\u0440\u044B\u0442\u044C \u043E\u043A\u043D\u043E\n"
        L"\u2022 \u041F\u0440\u0430\u0432\u0430\u044F \u043A\u043D\u043E\u043F\u043A\u0430 - \u043A\u043E\u043D\u0442\u0435\u043A\u0441\u0442\u043D\u043E\u0435 \u043C\u0435\u043D\u044E\n"
        L"\u2022 \u0417\u0430\u043A\u0440\u044B\u0442\u0438\u0435 \u043E\u043A\u043D\u0430 \u0441\u043A\u0440\u044B\u0432\u0430\u0435\u0442 \u0432 \u0442\u0440\u0435\u0439\n"
        L"\u2022 \u0424\u0430\u0439\u043B -> \u0412\u044B\u0445\u043E\u0434 - \u0437\u0430\u043A\u0440\u044B\u0442\u044C \u043F\u0440\u0438\u043B\u043E\u0436\u0435\u043D\u0438\u0435\n\n"
        L"\u0422\u043E\u043B\u044C\u043A\u043E \u043E\u0434\u0438\u043D \u044D\u043A\u0437\u0435\u043C\u043F\u043B\u044F\u0440 \u043F\u0440\u0438\u043B\u043E\u0436\u0435\u043D\u0438\u044F \u043C\u043E\u0436\u0435\u0442 \u0431\u044B\u0442\u044C \u0437\u0430\u043F\u0443\u0449\u0435\u043D!",
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        20, 30, 460, 200,
        g_hMainWnd, NULL, g_hInst, NULL);
}

HWND CreateHiddenWindow() {
    const wchar_t CLASS_NAME[] = L"TrayAppHiddenWindow";
    WNDCLASSW wc = {};
    wc.lpfnWndProc = HiddenWndProc;
    wc.hInstance = g_hInst;
    wc.lpszClassName = CLASS_NAME;
    RegisterClassW(&wc);
    
    return CreateWindowExW(0, CLASS_NAME, L"HiddenWindow",
        WS_POPUP, 0, 0, 0, 0,
        NULL, NULL, g_hInst, NULL);
}

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CLOSE:
        case WM_DESTROY:
            HideMainWindow();
            return 0;
        case WM_COMMAND:
            if (LOWORD(wParam) == ID_FILE_EXIT) {
                RemoveTrayIcon();
                ReleaseSingleInstance();
                PostQuitMessage(0);
                return 0;
            }
            break;
    }
    return DefWindowProcW(hWnd, message, wParam, lParam);
}

LRESULT CALLBACK HiddenWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    static UINT uTaskbarRestart = 0;
    
    if (uTaskbarRestart == 0) {
        uTaskbarRestart = RegisterWindowMessageW(L"TaskbarCreated");
    }
    
    if (message == WM_TRAYICON) {
        HandleTrayMessage(wParam, lParam);
        return 0;
    }
    
    if (uTaskbarRestart != 0 && message == uTaskbarRestart) {
        AddTrayIcon(hWnd);
        return 0;
    }
    
    switch (message) {
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_TRAY_OPEN:
                    ShowMainWindow();
                    return 0;
                case ID_TRAY_EXIT:
                    RemoveTrayIcon();
                    ReleaseSingleInstance();
                    PostQuitMessage(0);
                    return 0;
            }
            break;
    }
    return DefWindowProcW(hWnd, message, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    g_hInst = hInstance;
    
    if (!CheckSingleInstance()) {
        MessageBoxW(NULL, L"\u041F\u0440\u0438\u043B\u043E\u0436\u0435\u043D\u0438\u0435 \u0443\u0436\u0435 \u0437\u0430\u043F\u0443\u0449\u0435\u043D\u043E!", L"\u041E\u0448\u0438\u0431\u043A\u0430", MB_OK | MB_ICONERROR);
        return 1;
    }
    
    g_hHiddenWnd = CreateHiddenWindow();
    if (!g_hHiddenWnd) {
        ReleaseSingleInstance();
        return 1;
    }
    
    CreateMainWindow();
    AddTrayIcon(g_hHiddenWnd);
    
    MSG msg = {};
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    
    RemoveTrayIcon();
    if (g_hMainWnd) DestroyWindow(g_hMainWnd);
    if (g_hHiddenWnd) DestroyWindow(g_hHiddenWnd);
    ReleaseSingleInstance();
    
    return (int)msg.wParam;
}