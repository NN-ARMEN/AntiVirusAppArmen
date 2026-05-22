#pragma once
#include <windows.h>
#include <rpc.h>
#include <rpcdce.h>

#define RPC_PROTOCOL L"ncalrpc"
#define RPC_ENDPOINT L"AVAA_RPC"

BOOL CreateRpcBinding(void);
void DestroyRpcBinding(void);
BOOL CallStopService(void);
int CallGetServiceStatus(void);
void CallRegisterClient(long sessionId, long processId);
void CallUnregisterClient(long processId);