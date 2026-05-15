#include <windows.h>
#include <shellapi.h>
#include <tlhelp32.h>
#include <winsvc.h>
#include <rpc.h>

#include <malloc.h>

#include "resource.h"
#include "shared.h"
#include "tray_rpc.h"

namespace {

constexpr wchar_t kWindowClassName[] = L"ZIOVPOPracticeTrayWindow";
constexpr wchar_t kWindowTitle[] = L"ZIOVPO Practice 1";
constexpr wchar_t kMutexName[] = L"Local\\ZIOVPOPractice1TrayAppMutex";
constexpr UINT kTrayIconId = 1;
constexpr UINT kTrayMessage = WM_APP + 1;
constexpr UINT kMenuOpen = 50001;
constexpr UINT kMenuExit = 50002;

HINSTANCE g_instance = nullptr;
HWND g_main_window = nullptr;
UINT g_taskbar_created_message = 0;
bool g_tray_icon_added = false;

DWORD GetParentProcessId() {
    const DWORD current_pid = GetCurrentProcessId();
    DWORD parent_pid = 0;

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return 0;
    }

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);
    if (Process32FirstW(snapshot, &entry)) {
        do {
            if (entry.th32ProcessID == current_pid) {
                parent_pid = entry.th32ParentProcessID;
                break;
            }
        } while (Process32NextW(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return parent_pid;
}

bool QueryServiceStatus(SC_HANDLE service, SERVICE_STATUS_PROCESS& status) {
    DWORD bytes_needed = 0;
    return QueryServiceStatusEx(
        service,
        SC_STATUS_PROCESS_INFO,
        reinterpret_cast<LPBYTE>(&status),
        sizeof(status),
        &bytes_needed
    ) != FALSE;
}

bool WaitForServiceRunning(SC_HANDLE service) {
    SERVICE_STATUS_PROCESS status{};
    for (int i = 0; i < 60; ++i) {
        if (!QueryServiceStatus(service, status)) {
            return false;
        }
        if (status.dwCurrentState == SERVICE_RUNNING) {
            return true;
        }
        if (status.dwCurrentState == SERVICE_STOPPED) {
            return false;
        }
        Sleep(500);
    }
    return false;
}

bool EnsureServiceContextOrStartAndExit() {
    SC_HANDLE manager = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!manager) {
        return false;
    }

    SC_HANDLE service = OpenServiceW(manager, kServiceName, SERVICE_QUERY_STATUS | SERVICE_START);
    if (!service) {
        CloseServiceHandle(manager);
        return false;
    }

    SERVICE_STATUS_PROCESS status{};
    if (!QueryServiceStatus(service, status)) {
        CloseServiceHandle(service);
        CloseServiceHandle(manager);
        return false;
    }

    if (status.dwCurrentState == SERVICE_STOPPED) {
        StartServiceW(service, 0, nullptr);
        WaitForServiceRunning(service);
        CloseServiceHandle(service);
        CloseServiceHandle(manager);
        return false;
    }

    if (status.dwCurrentState != SERVICE_RUNNING) {
        WaitForServiceRunning(service);
        CloseServiceHandle(service);
        CloseServiceHandle(manager);
        return false;
    }

    const DWORD service_pid = status.dwProcessId;
    const DWORD parent_pid = GetParentProcessId();

    CloseServiceHandle(service);
    CloseServiceHandle(manager);

    return service_pid != 0 && parent_pid == service_pid;
}

bool StopServiceThroughRpc() {
    RPC_WSTR string_binding = nullptr;
    handle_t binding = nullptr;

    RPC_STATUS status = RpcStringBindingComposeW(
        nullptr,
        reinterpret_cast<RPC_WSTR>(const_cast<wchar_t*>(L"ncalrpc")),
        nullptr,
        reinterpret_cast<RPC_WSTR>(const_cast<wchar_t*>(kRpcEndpoint)),
        nullptr,
        &string_binding
    );
    if (status != RPC_S_OK) {
        return false;
    }

    status = RpcBindingFromStringBindingW(string_binding, &binding);
    RpcStringFreeW(&string_binding);
    if (status != RPC_S_OK) {
        return false;
    }

    bool sent = true;
    RpcTryExcept {
        StopService(binding);
    }
    RpcExcept(1) {
        sent = false;
    }
    RpcEndExcept

    RpcBindingFree(&binding);
    return sent;
}

void ShowMainWindow() {
    if (!g_main_window) {
        return;
    }

    ShowWindow(g_main_window, SW_SHOWNORMAL);
    SetForegroundWindow(g_main_window);
}

void RemoveTrayIcon() {
    if (!g_main_window || !g_tray_icon_added) {
        return;
    }

    NOTIFYICONDATA nid{};
    nid.cbSize = sizeof(nid);
    nid.hWnd = g_main_window;
    nid.uID = kTrayIconId;
    Shell_NotifyIcon(NIM_DELETE, &nid);
    g_tray_icon_added = false;
}

bool AddTrayIcon() {
    if (!g_main_window) {
        return false;
    }

    NOTIFYICONDATA nid{};
    nid.cbSize = sizeof(nid);
    nid.hWnd = g_main_window;
    nid.uID = kTrayIconId;
    nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    nid.uCallbackMessage = kTrayMessage;
    nid.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wcscpy_s(nid.szTip, L"ZIOVPO Practice 1");

    if (g_tray_icon_added) {
        if (!Shell_NotifyIcon(NIM_MODIFY, &nid) && !Shell_NotifyIcon(NIM_ADD, &nid)) {
            return false;
        }
    } else if (!Shell_NotifyIcon(NIM_ADD, &nid)) {
        return false;
    }

    nid.uVersion = NOTIFYICON_VERSION_4;
    Shell_NotifyIcon(NIM_SETVERSION, &nid);
    g_tray_icon_added = true;
    return true;
}

void ExitApplication() {
    StopServiceThroughRpc();
    RemoveTrayIcon();
    DestroyWindow(g_main_window);
    PostQuitMessage(0);
}

void ShowTrayMenu(HWND hwnd) {
    POINT cursor{};
    GetCursorPos(&cursor);

    HMENU menu = CreatePopupMenu();
    if (!menu) {
        return;
    }

    AppendMenu(menu, MF_STRING, kMenuOpen, L"Открыть");
    AppendMenu(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenu(menu, MF_STRING, kMenuExit, L"Выход");

    SetForegroundWindow(hwnd);
    TrackPopupMenu(menu, TPM_RIGHTBUTTON, cursor.x, cursor.y, 0, hwnd, nullptr);
    DestroyMenu(menu);
}

void AddMainWindowControls(HWND hwnd) {
    CreateWindowEx(
        0,
        L"STATIC",
        L"Приложение запущено. При закрытии окна оно продолжит работу в фоне.",
        WS_CHILD | WS_VISIBLE,
        20,
        20,
        520,
        24,
        hwnd,
        nullptr,
        g_instance,
        nullptr
    );

    CreateWindowEx(
        0,
        L"STATIC",
        L"Используйте иконку в трее или меню Файл -> Выход для завершения.",
        WS_CHILD | WS_VISIBLE,
        20,
        52,
        520,
        24,
        hwnd,
        nullptr,
        g_instance,
        nullptr
    );
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    if (message == g_taskbar_created_message) {
        AddTrayIcon();
        return 0;
    }

    switch (message) {
    case WM_CREATE:
        AddMainWindowControls(hwnd);
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wparam)) {
        case IDM_FILE_EXIT:
        case kMenuExit:
            ExitApplication();
            return 0;
        case kMenuOpen:
            ShowMainWindow();
            return 0;
        default:
            return 0;
        }

    case kTrayMessage:
        switch (LOWORD(lparam)) {
        case WM_LBUTTONUP:
            ShowMainWindow();
            return 0;
        case WM_RBUTTONUP:
        case WM_CONTEXTMENU:
            ShowTrayMenu(hwnd);
            return 0;
        default:
            return 0;
        }

    case WM_CLOSE:
        ShowWindow(hwnd, SW_HIDE);
        return 0;

    case WM_DESTROY:
        RemoveTrayIcon();
        return 0;

    default:
        return DefWindowProc(hwnd, message, wparam, lparam);
    }
}

bool IsBackgroundMode(LPWSTR command_line) {
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(command_line, &argc);
    if (!argv) {
        return false;
    }

    bool background = false;
    for (int i = 1; i < argc; ++i) {
        if (lstrcmpi(argv[i], L"--background") == 0 ||
            lstrcmpi(argv[i], L"--hidden") == 0 ||
            lstrcmpi(argv[i], L"/background") == 0 ||
            lstrcmpi(argv[i], L"/hidden") == 0) {
            background = true;
            break;
        }
    }

    LocalFree(argv);
    return background;
}

bool RegisterMainWindowClass() {
    WNDCLASS wc{};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = g_instance;
    wc.lpszClassName = kWindowClassName;
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszMenuName = MAKEINTRESOURCE(IDR_MAIN_MENU);

    return RegisterClass(&wc) != 0;
}

HWND CreateMainWindow() {
    return CreateWindowEx(
        0,
        kWindowClassName,
        kWindowTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        620,
        220,
        nullptr,
        nullptr,
        g_instance,
        nullptr
    );
}

} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, LPWSTR command_line, int show_command) {
    if (!EnsureServiceContextOrStartAndExit()) {
        return 0;
    }

    HANDLE single_instance_mutex = CreateMutex(nullptr, TRUE, kMutexName);
    if (!single_instance_mutex) {
        return 1;
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(single_instance_mutex);
        return 0;
    }

    g_instance = instance;
    g_taskbar_created_message = RegisterWindowMessage(L"TaskbarCreated");

    if (!RegisterMainWindowClass()) {
        CloseHandle(single_instance_mutex);
        return 1;
    }

    g_main_window = CreateMainWindow();
    if (!g_main_window) {
        CloseHandle(single_instance_mutex);
        return 1;
    }

    AddTrayIcon();

    if (!IsBackgroundMode(command_line)) {
        ShowWindow(g_main_window, show_command);
        UpdateWindow(g_main_window);
    }

    MSG message{};
    while (GetMessage(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }

    CloseHandle(single_instance_mutex);
    return static_cast<int>(message.wParam);
}

extern "C" void* __RPC_USER midl_user_allocate(size_t size) {
    return malloc(size);
}

extern "C" void __RPC_USER midl_user_free(void* pointer) {
    free(pointer);
}
