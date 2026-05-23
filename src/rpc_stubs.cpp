#include "rpc_interface.h"

// Заглушки для функций, которые нужны только для линковки AVAA.exe
void StopService(void) {}
int GetServiceStatus(void) { return 0; }
void RegisterClient(long, long) {}
void UnregisterClient(long) {}
void AuthLogin(const char*, const char*, BOOL*, char*, int) {}
void AuthLogout(BOOL*) {}
void GetCurrentUser(char*, int, BOOL*) {}
void GetLicenseStatus(char*, int, BOOL*, DWORD*) {}
void ActivateProduct(const char*, BOOL*, char*, int) {}
BOOL HasValidLicense(void) { return FALSE; }