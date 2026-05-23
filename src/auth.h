#pragma once
#include <windows.h>
#include <string>
#include <mutex>
#include <map>

BOOL InitAuthSystem();
void CleanupAuthSystem();
void StartBackgroundThreads();
void StopBackgroundThreads();

void AuthLogin(const char* username, const char* password, BOOL* success, char* errorMessage, int errorSize);
void AuthLogout(BOOL* success);
void GetCurrentUser(char* username, int usernameSize, BOOL* isAuthenticated);
void GetLicenseStatus(char* status, int statusSize, BOOL* isValid, DWORD* validUntil);
void ActivateProduct(const char* activationCode, BOOL* success, char* errorMessage, int errorSize);
BOOL HasValidLicense();