#pragma once
#include <windows.h>
#include <rpc.h>
#include <rpcdce.h>

#define RPC_PROTOCOL L"ncalrpc"
#define RPC_ENDPOINT L"AVAA_RPC"

// Базовые RPC функции
void StopService(void);
int GetServiceStatus(void);
void RegisterClient(long sessionId, long processId);
void UnregisterClient(long processId);

// RPC функции аутентификации и лицензирования (объявления)
void AuthLogin(const char* username, const char* password, BOOL* success, char* errorMessage, int errorSize);
void AuthLogout(BOOL* success);
void GetCurrentUser(char* username, int usernameSize, BOOL* isAuthenticated);
void GetLicenseStatus(char* status, int statusSize, BOOL* isValid, DWORD* validUntil);
void ActivateProduct(const char* activationCode, BOOL* success, char* errorMessage, int errorSize);
BOOL HasValidLicense(void);

// Клиентские функции (для AVAA.exe)
BOOL CreateRpcBinding(void);
void DestroyRpcBinding(void);
BOOL CallStopService(void);
int CallGetServiceStatus(void);
void CallRegisterClient(long sessionId, long processId);
void CallUnregisterClient(long processId);
BOOL CallAuthLogin(const char* username, const char* password, char* errorMessage, int errorSize);
void CallAuthLogout(void);
BOOL CallGetCurrentUser(char* username, int usernameSize, BOOL* isAuthenticated);
BOOL CallGetLicenseStatus(char* status, int statusSize, BOOL* isValid, DWORD* validUntil);
BOOL CallActivateProduct(const char* activationCode, char* errorMessage, int errorSize);
BOOL CallHasValidLicense(void);

// Серверные функции (для службы)
BOOL StartRpcServer(HANDLE hStopEvent);
void StopRpcServer(void);

extern handle_t g_hBinding;