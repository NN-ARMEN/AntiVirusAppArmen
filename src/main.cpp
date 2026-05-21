#include <windows.h>
#include <shellapi.h>
#include <winsvc.h>
#include <tlhelp32.h>
#include <string>
#include "rpc_interface.h"

extern BOOL CreateRpcBinding(void);
extern void DestroyRpcBinding(void);
extern BOOL CallStopService(void);
extern int CallGetServiceStatus(void);
extern void CallRegisterClient(long sessionId, long processId);
extern void CallUnregisterClient(long processId);

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "advapi32.lib")

#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_OPEN 1001
#define ID_TRAY_EXIT 1002
#define ID_FILE_EXIT 2001
#define SERVICE_NAME L"AVAA_Service"
#define SERVICE_START_TIMEOUT 30000

HINSTANCE g_hInst = NULL;
HWND g_hMainWnd = NULL;
HWND g_hHiddenWnd = NULL;
NOTIFYICONDATAW g_nid = {};
BOOL g_bWindowVisible = FALSE;
HANDLE g_hMutex = NULL;
const wchar_t MUTEX_NAME[] = L"Global\\AVAA_Single_Instance";

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
BOOL IsServiceRunning();
BOOL StartServiceAndWait();
BOOL IsParentService();
void StopServiceViaRPC();
void RegisterWithService();

BOOL IsServiceRunning() {
    SC_HANDLE scManager = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
    if (!scManager) return FALSE;
    SC_HANDLE scService = OpenServiceW(scManager, SERVICE_NAME, SERVICE_QUERY_STATUS);
    if (!scService) {
        CloseServiceHandle(scManager);
        return FALSE;
    }
    SERVICE_STATUS_PROCESS ssStatus;
    DWORD bytesNeeded;
    BOOL result = FALSE;
    if (QueryServiceStatusEx(scService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssStatus, sizeof(ssStatus), &bytesNeeded)) {
        result = (ssStatus.dwCurrentState == SERVICE_RUNNING);
    }
    CloseServiceHandle(scService);
    CloseServiceHandle(scManager);
    return result;
}

BOOL StartServiceAndWait() {
    SC_HANDLE scManager = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
    if (!scManager) return FALSE;
    SC_HANDLE scService = OpenServiceW(scManager, SERVICE_NAME, SERVICE_START | SERVICE_QUERY_STATUS);
    if (!scService) {
        CloseServiceHandle(scManager);
        return FALSE;
    }
    if (!StartServiceW(scService, 0, NULL)) {
        if (GetLastError() != ERROR_SERVICE_ALREADY_RUNNING) {
            CloseServiceHandle(scService);
            CloseServiceHandle(scManager);
            return FALSE;
        }
    }
    SERVICE_STATUS_PROCESS ssStatus;
    DWORD startTime = GetTickCount();
    DWORD bytesNeeded;
    while (GetTickCount() - startTime < SERVICE_START_TIMEOUT) {
        if (QueryServiceStatusEx(scService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssStatus, sizeof(ssStatus), &bytesNeeded)) {
            if (ssStatus.dwCurrentState == SERVICE_RUNNING) {
                CloseServiceHandle(scService);
                CloseServiceHandle(scManager);
                return TRUE;
            }
        }
        Sleep(1000);
    }
    CloseServiceHandle(scService);
    CloseServiceHandle(scManager);
    return FALSE;
}

BOOL IsParentService() {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return FALSE;
    PROCESSENTRY32W pe = { sizeof(pe) };
    DWORD currentPid = GetCurrentProcessId();
    DWORD parentPid = 0;
    if (Process32FirstW(hSnapshot, &pe)) {
        do {
            if (pe.th32ProcessID == currentPid) {
                parentPid = pe.th32ParentProcessID;
                break;
            }
        } while (Process32NextW(hSnapshot, &pe));
    }
    CloseHandle(hSnapshot);
    if (parentPid == 0) return FALSE;
    HANDLE hParent = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, parentPid);
    if (!hParent) return FALSE;
    WCHAR parentPath[MAX_PATH];
    DWORD size = MAX_PATH;
    BOOL result = FALSE;
    if (QueryFullProcessImageNameW(hParent, 0, parentPath, &size)) {
        result = (wcsstr(parentPath, L"AVAAService") != NULL) || (wcsstr(parentPath, L"AVAA_Service") != NULL);
    }
    CloseHandle(hParent);
    return result;
}

void StopServiceViaRPC() {
    if (CreateRpcBinding()) {
        CallStopService();
        DestroyRpcBinding();
    }
}

void RegisterWithService() {
    DWORD sessionId = 0;
    ProcessIdToSessionId(GetCurrentProcessId(), &sessionId);
    if (CreateRpcBinding()) {
        CallRegisterClient(sessionId, GetCurrentProcessId());
        DestroyRpcBinding();
    }
}

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
    wcscpy_s(g_nid.szTip, 128, L"AVAA");
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
    InsertMenuW(hMenu, 0, MF_BYPOSITION | MF_STRING, ID_TRAY_OPEN, L"Открыть");
    InsertMenuW(hMenu, 1, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
    InsertMenuW(hMenu, 2, MF_BYPOSITION | MF_STRING, ID_TRAY_EXIT, L"Выход");
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
    const wchar_t CLASS_NAME[] = L"AVAA_MainWindow";
    WNDCLASSW wc = {};
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = g_hInst;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassW(&wc);
    g_hMainWnd = CreateWindowExW(0, CLASS_NAME, L"AVAA", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 550, 450, NULL, NULL, g_hInst, NULL);
    HMENU hMenuBar = CreateMenu();
    HMENU hFileMenu = CreatePopupMenu();
    AppendMenuW(hFileMenu, MF_STRING, ID_FILE_EXIT, L"Выход");
    AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hFileMenu, L"Файл");
    SetMenu(g_hMainWnd, hMenuBar);
    CreateWindowW(L"STATIC", L"AVAA - Приложение работает в системном трее\n\n• Левая кнопка на иконке - показать/скрыть окно\n• Правая кнопка - контекстное меню\n• Закрытие окна скрывает в трей\n• Выход через меню останавливает службу\n\nТолько один экземпляр приложения может быть запущен!", WS_VISIBLE | WS_CHILD | SS_LEFT, 20, 30, 500, 180, g_hMainWnd, NULL, g_hInst, NULL);
}

HWND CreateHiddenWindow() {
    const wchar_t CLASS_NAME[] = L"AVAA_HiddenWindow";
    WNDCLASSW wc = {};
    wc.lpfnWndProc = HiddenWndProc;
    wc.hInstance = g_hInst;
    wc.lpszClassName = CLASS_NAME;
    RegisterClassW(&wc);
    return CreateWindowExW(0, CLASS_NAME, L"HiddenWindow", WS_POPUP, 0, 0, 0, 0, NULL, NULL, g_hInst, NULL);
}

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CLOSE: case WM_DESTROY:
            HideMainWindow();
            return 0;
        case WM_COMMAND:
            if (LOWORD(wParam) == ID_FILE_EXIT) {
                StopServiceViaRPC();
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
    if (uTaskbarRestart == 0) uTaskbarRestart = RegisterWindowMessageW(L"TaskbarCreated");
    if (message == WM_TRAYICON) { HandleTrayMessage(wParam, lParam); return 0; }
    if (uTaskbarRestart != 0 && message == uTaskbarRestart) { AddTrayIcon(hWnd); return 0; }
    switch (message) {
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_TRAY_OPEN: ShowMainWindow(); return 0;
                case ID_TRAY_EXIT:
                    StopServiceViaRPC();
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
    
    // ================================================
    // ВРЕМЕННО ОТКЛЮЧЕНО ДЛЯ ТЕСТА ИКОНКИ
    // ================================================
    // if (!IsParentService()) {
    //     if (!IsServiceRunning()) StartServiceAndWait();
    //     return 0;
    // }
    // ================================================
    
    if (!CheckSingleInstance()) {
        MessageBoxW(NULL, L"AVAA уже запущен!", L"Ошибка", MB_OK | MB_ICONERROR);
        return 1;
    }
    
    RegisterWithService();
    g_hHiddenWnd = CreateHiddenWindow();
    if (!g_hHiddenWnd) { ReleaseSingleInstance(); return 1; }
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