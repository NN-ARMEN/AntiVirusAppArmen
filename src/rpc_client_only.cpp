#include "rpc_interface.h"
#include <stdlib.h>

handle_t g_hBinding = NULL;

void* __RPC_USER MIDL_user_allocate(size_t size) {
    return malloc(size);
}

void __RPC_USER MIDL_user_free(void* p) {
    free(p);
}

BOOL CreateRpcBinding(void) {
    RPC_STATUS status;
    RPC_WSTR stringBinding = NULL;
    
// Ждём, пока служба запустится
    Sleep(1000);

    status = RpcStringBindingComposeW(NULL, (RPC_WSTR)RPC_PROTOCOL, NULL, (RPC_WSTR)RPC_ENDPOINT, NULL, &stringBinding);
    if (status != RPC_S_OK) return FALSE;
    
    status = RpcBindingFromStringBindingW(stringBinding, &g_hBinding);
    RpcStringFreeW(&stringBinding);
    return (status == RPC_S_OK);
}

void DestroyRpcBinding(void) {
    if (g_hBinding) {
        RpcBindingFree(&g_hBinding);
        g_hBinding = NULL;
    }
}

BOOL CallStopService(void) {
    if (!g_hBinding) return FALSE;
    __try { StopService(); return TRUE; }
    __except(EXCEPTION_EXECUTE_HANDLER) { return FALSE; }
}

int CallGetServiceStatus(void) {
    if (!g_hBinding) return -1;
    __try { return GetServiceStatus(); }
    __except(EXCEPTION_EXECUTE_HANDLER) { return -1; }
}

void CallRegisterClient(long sessionId, long processId) {
    if (!g_hBinding) return;
    __try { RegisterClient(sessionId, processId); }
    __except(EXCEPTION_EXECUTE_HANDLER) {}
}

void CallUnregisterClient(long processId) {
    if (!g_hBinding) return;
    __try { UnregisterClient(processId); }
    __except(EXCEPTION_EXECUTE_HANDLER) {}
}

BOOL CallAuthLogin(const char* username, const char* password, char* errorMessage, int errorSize) {
    if (!g_hBinding) return FALSE;
    BOOL success = FALSE;
    __try { AuthLogin(username, password, &success, errorMessage, errorSize); return success; }
    __except(EXCEPTION_EXECUTE_HANDLER) { return FALSE; }
}

void CallAuthLogout(void) {
    if (!g_hBinding) return;
    BOOL success = FALSE;
    __try { AuthLogout(&success); }
    __except(EXCEPTION_EXECUTE_HANDLER) {}
}

BOOL CallGetCurrentUser(char* username, int usernameSize, BOOL* isAuthenticated) {
    if (!g_hBinding) return FALSE;
    __try { GetCurrentUser(username, usernameSize, isAuthenticated); return TRUE; }
    __except(EXCEPTION_EXECUTE_HANDLER) { return FALSE; }
}

BOOL CallGetLicenseStatus(char* status, int statusSize, BOOL* isValid, DWORD* validUntil) {
    if (!g_hBinding) return FALSE;
    __try { GetLicenseStatus(status, statusSize, isValid, validUntil); return TRUE; }
    __except(EXCEPTION_EXECUTE_HANDLER) { return FALSE; }
}

BOOL CallActivateProduct(const char* activationCode, char* errorMessage, int errorSize) {
    if (!g_hBinding) return FALSE;
    BOOL success = FALSE;
    __try { ActivateProduct(activationCode, &success, errorMessage, errorSize); return success; }
    __except(EXCEPTION_EXECUTE_HANDLER) { return FALSE; }
}

BOOL CallHasValidLicense(void) {
    if (!g_hBinding) return FALSE;
    __try { return HasValidLicense(); }
    __except(EXCEPTION_EXECUTE_HANDLER) { return FALSE; }
}