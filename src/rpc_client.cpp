#include <windows.h>
#include <rpc.h>

#include <malloc.h>

#include "shared.h"

extern "C" {
#include "tray_rpc.h"
}

bool SendStopServiceRequest() {
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

extern "C" void* __RPC_USER midl_user_allocate(size_t size) {
    return malloc(size);
}

extern "C" void __RPC_USER midl_user_free(void* pointer) {
    free(pointer);
}

