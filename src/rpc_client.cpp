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

// Для клиента эти функции не нужны, но они должны существовать
// Просто заглушки, которые ничего не делают
void StopService(void) {}
int GetServiceStatus(void) { return 0; }
void RegisterClient(long sessionId, long processId) { (void)sessionId; (void)processId; }
void UnregisterClient(long processId) { (void)processId; }

// Функции для вызова RPC (отправка сообщений на сервер)
BOOL CallStopService(void) {
    if (!g_hBinding) return FALSE;
    // Здесь должна быть реальная RPC отправка
    return TRUE;
}

int CallGetServiceStatus(void) {
    if (!g_hBinding) return -1;
    return 1;
}

void CallRegisterClient(long sessionId, long processId) {
    if (!g_hBinding) return;
    (void)sessionId; (void)processId;
}

void CallUnregisterClient(long processId) {
    if (!g_hBinding) return;
    (void)processId;
}