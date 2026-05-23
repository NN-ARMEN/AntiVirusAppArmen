#include "auth.h"
#include <string>
#include <mutex>

static struct {
    std::string username;
    bool isAuthenticated;
} g_authData = {"", false};

static std::mutex g_authMutex;
static BOOL g_initialized = FALSE;

BOOL InitAuthSystem() {
    g_initialized = TRUE;
    return TRUE;
}

void CleanupAuthSystem() {
    g_initialized = FALSE;
}

void StartBackgroundThreads() {}
void StopBackgroundThreads() {}

void AuthLogin(const char* username, const char* password, BOOL* success, char* errorMessage, int errorSize) {
    // Для теста принимаем любые admin/admin
    if (strcmp(username, "admin") == 0 && strcmp(password, "admin") == 0) {
        std::lock_guard<std::mutex> lock(g_authMutex);
        g_authData.username = username;
        g_authData.isAuthenticated = true;
        *success = TRUE;
        if (errorMessage) strcpy_s(errorMessage, errorSize, "");
    } else {
        *success = FALSE;
        if (errorMessage) strcpy_s(errorMessage, errorSize, "Invalid username or password");
    }
}

void AuthLogout(BOOL* success) {
    std::lock_guard<std::mutex> lock(g_authMutex);
    g_authData = {"", false};
    *success = TRUE;
}

void GetCurrentUser(char* username, int usernameSize, BOOL* isAuthenticated) {
    std::lock_guard<std::mutex> lock(g_authMutex);
    *isAuthenticated = g_authData.isAuthenticated;
    if (username && g_authData.isAuthenticated) {
        strcpy_s(username, usernameSize, g_authData.username.c_str());
    } else if (username) {
        username[0] = 0;
    }
}

void GetLicenseStatus(char* status, int statusSize, BOOL* isValid, DWORD* validUntil) {
    *isValid = TRUE;
    *validUntil = (DWORD)time(NULL) + 30 * 86400; // 30 дней
    if (status) strcpy_s(status, statusSize, "active");
}

void ActivateProduct(const char* activationCode, BOOL* success, char* errorMessage, int errorSize) {
    // Любой код активации принимаем
    *success = TRUE;
    if (errorMessage) strcpy_s(errorMessage, errorSize, "");
}

BOOL HasValidLicense() {
    return TRUE;
}