#include "rpc_interface.h"
#include <stdlib.h>

// Заглушки для функций, которые нужны только клиенту
// Эти функции никогда не будут вызваны на клиенте, 
// но нужны для линковки

void StopService(void) {
    // Заглушка - никогда не вызывается на клиенте
}

int GetServiceStatus(void) {
    // Заглушка
    return 0;
}

void RegisterClient(long sessionId, long processId) {
    (void)sessionId;
    (void)processId;
}

void UnregisterClient(long processId) {
    (void)processId;
}

void AuthLogin(const char* username, const char* password, BOOL* success, char* errorMessage, int errorSize) {
    (void)username;
    (void)password;
    (void)success;
    (void)errorMessage;
    (void)errorSize;
}

void AuthLogout(BOOL* success) {
    (void)success;
}

void GetCurrentUser(char* username, int usernameSize, BOOL* isAuthenticated) {
    if (username && usernameSize > 0) username[0] = 0;
    if (isAuthenticated) *isAuthenticated = FALSE;
}

void GetLicenseStatus(char* status, int statusSize, BOOL* isValid, DWORD* validUntil) {
    if (status && statusSize > 0) status[0] = 0;
    if (isValid) *isValid = FALSE;
    if (validUntil) *validUntil = 0;
}

void ActivateProduct(const char* activationCode, BOOL* success, char* errorMessage, int errorSize) {
    (void)activationCode;
    if (success) *success = FALSE;
    if (errorMessage && errorSize > 0) {
        strncpy_s(errorMessage, errorSize, "Not implemented in client", errorSize - 1);
    }
}

BOOL HasValidLicense(void) {
    return FALSE;
}