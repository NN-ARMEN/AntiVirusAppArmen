#pragma once
#include <windows.h>
#include <rpc.h>
#include <rpcdce.h>

#define AVAA_RPC_UUID \
    {0x12345678, 0x1234, 0x1234, {0x12, 0x34, 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC}}

#define RPC_PROTOCOL L"ncalrpc"
#define RPC_ENDPOINT L"AVAA_RPC"

// RPC функции
void StopService(void);
int GetServiceStatus(void);
void RegisterClient(long sessionId, long processId);
void UnregisterClient(long processId);

// Клиентские функции
BOOL CreateRpcBinding(void);
void DestroyRpcBinding(void);
BOOL CallStopService(void);
int CallGetServiceStatus(void);
void CallRegisterClient(long sessionId, long processId);
void CallUnregisterClient(long processId);

// Серверные функции
BOOL StartRpcServer(HANDLE hStopEvent);
void StopRpcServer(void);

extern handle_t g_hBinding;