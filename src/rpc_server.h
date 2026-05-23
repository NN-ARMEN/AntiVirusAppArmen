#pragma once
#include <windows.h>

BOOL StartRpcServer(HANDLE hStopEvent);
void StopRpcServer(void);