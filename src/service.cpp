#include <windows.h>
#include <winsvc.h>
#include <userenv.h>
#include <wtsapi32.h>
#include <tlhelp32.h>
#include <vector>
#include <string>

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "userenv.lib")
#pragma comment(lib, "wtsapi32.lib")
#pragma comment(lib, "rpcrt4.lib")

#define SERVICE_NAME L"AVAA_Service"
#define DISPLAY_NAME L"AVAA Tray Service"

#ifndef WTS_SESSION_LOGON
#define WTS_SESSION_LOGON 5
#endif

struct ClientInfo {
    DWORD sessionId;
    DWORD processId;
    HANDLE processHandle;
};

std::vector<ClientInfo> g_clients;
CRITICAL_SECTION g_csClients;
SERVICE_STATUS g_serviceStatus;
SERVICE_STATUS_HANDLE g_serviceStatusHandle;
HANDLE g_hStopEvent = NULL;
HANDLE g_hRpcThread = NULL;

void WINAPI ServiceMain(DWORD argc, LPWSTR* argv);
DWORD WINAPI ServiceCtrlHandlerEx(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext);
DWORD WINAPI ServiceWorkerThread(LPVOID lpParam);
DWORD WINAPI RpcServerThread(LPVOID lpParam);
BOOL StartClientApp(DWORD sessionId);
void StopAllClients();
void EnumerateAndStartClients();
BOOL SetPrivilege(HANDLE hToken, LPCWSTR lpszPrivilege, BOOL bEnable);

extern BOOL StartRpcServer(HANDLE hStopEvent);
extern void StopRpcServer();
extern void RegisterClient(long sessionId, long processId);
extern void UnregisterClient(long processId);

BOOL SetPrivilege(HANDLE hToken, LPCWSTR lpszPrivilege, BOOL bEnable) {
    TOKEN_PRIVILEGES tp;
    LUID luid;
    
    if (!LookupPrivilegeValueW(NULL, lpszPrivilege, &luid)) {
        return FALSE;
    }
    
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = bEnable ? SE_PRIVILEGE_ENABLED : 0;
    
    if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL)) {
        return FALSE;
    }
    
    return GetLastError() == ERROR_SUCCESS;
}

void RegisterClient(long sessionId, long processId) {
    EnterCriticalSection(&g_csClients);
    BOOL exists = FALSE;
    for (auto& client : g_clients) {
        if (client.processId == (DWORD)processId) {
            exists = TRUE;
            break;
        }
    }
    if (!exists) {
        ClientInfo info;
        info.sessionId = (DWORD)sessionId;
        info.processId = (DWORD)processId;
        info.processHandle = NULL;
        g_clients.push_back(info);
    }
    LeaveCriticalSection(&g_csClients);
}

void UnregisterClient(long processId) {
    EnterCriticalSection(&g_csClients);
    for (auto it = g_clients.begin(); it != g_clients.end(); ++it) {
        if (it->processId == (DWORD)processId) {
            if (it->processHandle) {
                CloseHandle(it->processHandle);
            }
            g_clients.erase(it);
            break;
        }
    }
    LeaveCriticalSection(&g_csClients);
}

BOOL StartClientApp(DWORD sessionId) {
    HANDLE hUserToken = NULL;
    HANDLE hDuplicatedToken = NULL;
    
    if (!WTSQueryUserToken(sessionId, &hUserToken)) {
        return FALSE;
    }
    
    WCHAR appPath[MAX_PATH];
    GetModuleFileNameW(NULL, appPath, MAX_PATH);
    
    WCHAR* lastSlash = wcsrchr(appPath, L'\\');
    if (lastSlash) {
        *(lastSlash + 1) = L'\0';
        wcscat_s(appPath, L"AVAA.exe");
    }
    
    if (GetFileAttributesW(appPath) == INVALID_FILE_ATTRIBUTES) {
        CloseHandle(hUserToken);
        return FALSE;
    }
    
    if (!DuplicateTokenEx(hUserToken, TOKEN_ALL_ACCESS, NULL, 
        SecurityImpersonation, TokenPrimary, &hDuplicatedToken)) {
        CloseHandle(hUserToken);
        return FALSE;
    }
    
    SetPrivilege(hDuplicatedToken, SE_INCREASE_QUOTA_NAME, TRUE);
    
    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi = { 0 };
    si.lpDesktop = L"winsta0\\default";
    si.wShowWindow = SW_HIDE;
    si.dwFlags = STARTF_USESHOWWINDOW;
    
    BOOL result = CreateProcessAsUserW(
        hDuplicatedToken,
        appPath,
        NULL,
        NULL, NULL,
        FALSE,
        CREATE_NO_WINDOW,
        NULL, NULL,
        &si, &pi
    );
    
    if (result) {
        EnterCriticalSection(&g_csClients);
        ClientInfo info;
        info.sessionId = sessionId;
        info.processId = pi.dwProcessId;
        info.processHandle = pi.hProcess;
        g_clients.push_back(info);
        LeaveCriticalSection(&g_csClients);
        CloseHandle(pi.hThread);
    }
    
    CloseHandle(hDuplicatedToken);
    CloseHandle(hUserToken);
    return result;
}

void StopAllClients() {
    EnterCriticalSection(&g_csClients);
    for (auto& client : g_clients) {
        if (client.processHandle) {
            TerminateProcess(client.processHandle, 0);
            CloseHandle(client.processHandle);
        }
    }
    g_clients.clear();
    LeaveCriticalSection(&g_csClients);
}

void EnumerateAndStartClients() {
    DWORD sessionCount = 0;
    WTS_SESSION_INFOW* pSessionInfo = NULL;
    
    if (WTSEnumerateSessionsW(WTS_CURRENT_SERVER_HANDLE, 0, 1, &pSessionInfo, &sessionCount)) {
        for (DWORD i = 0; i < sessionCount; i++) {
            if (pSessionInfo[i].State == WTSActive && pSessionInfo[i].SessionId != 0) {
                BOOL alreadyRunning = FALSE;
                EnterCriticalSection(&g_csClients);
                for (auto& client : g_clients) {
                    if (client.sessionId == pSessionInfo[i].SessionId) {
                        alreadyRunning = TRUE;
                        break;
                    }
                }
                LeaveCriticalSection(&g_csClients);
                
                if (!alreadyRunning) {
                    StartClientApp(pSessionInfo[i].SessionId);
                }
            }
        }
        WTSFreeMemory(pSessionInfo);
    }
}

DWORD WINAPI RpcServerThread(LPVOID lpParam) {
    StartRpcServer(g_hStopEvent);
    return 0;
}

DWORD WINAPI ServiceWorkerThread(LPVOID lpParam) {
    g_hRpcThread = CreateThread(NULL, 0, RpcServerThread, NULL, 0, NULL);
    
    EnumerateAndStartClients();
    
    g_serviceStatus.dwCurrentState = SERVICE_RUNNING;
    SetServiceStatus(g_serviceStatusHandle, &g_serviceStatus);
    
    while (WaitForSingleObject(g_hStopEvent, 5000) == WAIT_TIMEOUT) {
        EnumerateAndStartClients();
    }
    
    StopAllClients();
    StopRpcServer();
    
    if (g_hRpcThread) {
        WaitForSingleObject(g_hRpcThread, 5000);
        CloseHandle(g_hRpcThread);
    }
    
    g_serviceStatus.dwCurrentState = SERVICE_STOPPED;
    SetServiceStatus(g_serviceStatusHandle, &g_serviceStatus);
    
    return 0;
}

DWORD WINAPI ServiceCtrlHandlerEx(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext) {
    switch (dwControl) {
        case SERVICE_CONTROL_STOP:
        case SERVICE_CONTROL_SHUTDOWN:
            return NO_ERROR;
            
        case SERVICE_CONTROL_SESSIONCHANGE:
            {
                DWORD sessionId = dwEventType;
                DWORD eventType = (DWORD)(ULONG_PTR)lpEventData;
                
                if (eventType == WTS_SESSION_LOGON) {
                    if (sessionId != 0) {
                        Sleep(3000);
                        StartClientApp(sessionId);
                    }
                }
            }
            break;
    }
    
    SetServiceStatus(g_serviceStatusHandle, &g_serviceStatus);
    return NO_ERROR;
}

void WINAPI ServiceMain(DWORD argc, LPWSTR* argv) {
    InitializeCriticalSection(&g_csClients);
    
    g_serviceStatusHandle = RegisterServiceCtrlHandlerExW(
        SERVICE_NAME,
        ServiceCtrlHandlerEx,
        NULL
    );
    
    if (!g_serviceStatusHandle) {
        DeleteCriticalSection(&g_csClients);
        return;
    }
    
    g_serviceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_serviceStatus.dwCurrentState = SERVICE_START_PENDING;
    g_serviceStatus.dwControlsAccepted = 0;
    g_serviceStatus.dwWin32ExitCode = NO_ERROR;
    g_serviceStatus.dwServiceSpecificExitCode = 0;
    g_serviceStatus.dwCheckPoint = 0;
    g_serviceStatus.dwWaitHint = 30000;
    
    SetServiceStatus(g_serviceStatusHandle, &g_serviceStatus);
    
    g_hStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    
    HANDLE hThread = CreateThread(NULL, 0, ServiceWorkerThread, NULL, 0, NULL);
    WaitForSingleObject(hThread, INFINITE);
    
    CloseHandle(hThread);
    CloseHandle(g_hStopEvent);
    DeleteCriticalSection(&g_csClients);
}

int main(int argc, char* argv[]) {
    SERVICE_TABLE_ENTRYW serviceTable[] = {
        { (LPWSTR)SERVICE_NAME, (LPSERVICE_MAIN_FUNCTIONW)ServiceMain },
        { NULL, NULL }
    };
    
    StartServiceCtrlDispatcherW(serviceTable);
    return 0;
}