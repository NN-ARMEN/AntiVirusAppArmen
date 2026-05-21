#include "rpc_interface.h"
#include <stdlib.h>

HANDLE g_hStopEventForRPC = NULL;

void* __RPC_USER MIDL_user_allocate(size_t size) {
    return malloc(size);
}

void __RPC_USER MIDL_user_free(void* p) {
    free(p);
}

void StopService(void) {
    if (g_hStopEventForRPC) {
        SetEvent(g_hStopEventForRPC);
    }
}

int GetServiceStatus(void) {
    if (g_hStopEventForRPC && WaitForSingleObject(g_hStopEventForRPC, 0) == WAIT_TIMEOUT) {
        return 1;
    }
    return 0;
}

// Убираем RegisterClient и UnregisterClient отсюда - они в service.cpp

BOOL StartRpcServer(HANDLE hStopEvent) {
    RPC_STATUS status;
    
    g_hStopEventForRPC = hStopEvent;
    
    status = RpcServerUseProtseqEpW(
        (RPC_WSTR)RPC_PROTOCOL,
        RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
        (RPC_WSTR)RPC_ENDPOINT,
        NULL
    );
    
    if (status != RPC_S_OK) {
        return FALSE;
    }
    
    status = RpcServerRegisterIfEx(
        NULL,
        NULL,
        NULL,
        RPC_IF_ALLOW_CALLBACKS_WITH_NO_AUTH,
        0,
        NULL
    );
    
    if (status != RPC_S_OK) {
        return FALSE;
    }
    
    status = RpcServerListen(1, RPC_C_LISTEN_MAX_CALLS_DEFAULT, FALSE);
    
    return (status == RPC_S_OK);
}

void StopRpcServer(void) {
    RpcMgmtStopServerListening(NULL);
    RpcServerUnregisterIf(NULL, NULL, FALSE);
}