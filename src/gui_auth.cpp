#include <windows.h>
#include <string>
#include "rpc_interface.h"

// Структура информации о пользователе
struct UserInfo {
    std::string username;
    BOOL isAuthenticated;
    BOOL hasLicense;
    std::string licenseStatus;
    DWORD licenseValidUntil;
};

// Функции для работы с RPC (обёртки)
static BOOL CallGetCurrentUserInternal(char* username, int usernameSize, BOOL* isAuthenticated) {
    return CallGetCurrentUser(username, usernameSize, isAuthenticated);
}

static BOOL CallGetLicenseStatusInternal(char* status, int statusSize, BOOL* isValid, DWORD* validUntil) {
    return CallGetLicenseStatus(status, statusSize, isValid, validUntil);
}

static BOOL CallAuthLoginInternal(const char* username, const char* password, char* errorMessage, int errorSize) {
    return CallAuthLogin(username, password, errorMessage, errorSize);
}

static void CallAuthLogoutInternal() {
    CallAuthLogout();
}

static BOOL CallActivateProductInternal(const char* activationCode, char* errorMessage, int errorSize) {
    return CallActivateProduct(activationCode, errorMessage, errorSize);
}

// Получение информации о текущем пользователе
UserInfo GetCurrentUserInfo() {
    UserInfo info = {};
    info.isAuthenticated = FALSE;
    info.hasLicense = FALSE;
    
    char username[256] = {0};
    BOOL isAuthenticated = FALSE;
    
    if (CallGetCurrentUserInternal(username, sizeof(username), &isAuthenticated)) {
        info.username = username;
        info.isAuthenticated = isAuthenticated;
        
        if (isAuthenticated) {
            char status[64] = {0};
            BOOL isValid = FALSE;
            DWORD validUntil = 0;
            
            if (CallGetLicenseStatusInternal(status, sizeof(status), &isValid, &validUntil)) {
                info.hasLicense = isValid;
                info.licenseStatus = status;
                info.licenseValidUntil = validUntil;
            }
        }
    }
    
    return info;
}

// Диалог аутентификации
BOOL ShowLoginDialog(HWND hParent, UserInfo& userInfo) {
    char username[256] = {0};
    char password[256] = {0};
    char errorMsg[512] = {0};
    
    // Создание простого диалога через MessageBox (в реальном приложении нужен полноценный диалог)
    // Здесь упрощённая версия - ввод через консоль для тестирования
    
    // Для реального GUI нужно создать диалоговое окно с полями ввода
    
    // Возвращаем результат
    if (CallAuthLoginInternal(username, password, errorMsg, sizeof(errorMsg))) {
        userInfo = GetCurrentUserInfo();
        return TRUE;
    }
    
    MessageBoxA(hParent, errorMsg, "Authentication Error", MB_OK | MB_ICONERROR);
    return FALSE;
}

// Форма активации
BOOL ShowActivationDialog(HWND hParent, UserInfo& userInfo) {
    char activationCode[256] = {0};
    char errorMsg[512] = {0};
    
    // Упрощённая версия
    if (CallActivateProductInternal(activationCode, errorMsg, sizeof(errorMsg))) {
        userInfo = GetCurrentUserInfo();
        return TRUE;
    }
    
    MessageBoxA(hParent, errorMsg, "Activation Error", MB_OK | MB_ICONERROR);
    return FALSE;
}

// Обновление состояния лицензии на главном окне
void UpdateLicenseStatus(HWND hWnd, const UserInfo& userInfo) {
    char statusText[512] = {0};
    
    if (userInfo.isAuthenticated && userInfo.hasLicense) {
        // Антивирусная функциональность разблокирована
        sprintf_s(statusText, "Licensed until: %s", 
                  std::to_string(userInfo.licenseValidUntil).c_str());
        SetWindowTextA(hWnd, statusText);
    } else if (userInfo.isAuthenticated && !userInfo.hasLicense) {
        SetWindowTextA(hWnd, "No license - Please activate");
    } else {
        SetWindowTextA(hWnd, "Not authenticated - Please login");
    }
}

// Поток периодического опроса состояния лицензии
DWORD WINAPI LicensePollingThread(LPVOID lpParam) {
    HWND hWnd = (HWND)lpParam;
    
    while (true) {
        Sleep(10000); // Опрашиваем каждые 10 секунд
        
        UserInfo userInfo = GetCurrentUserInfo();
        
        if (userInfo.isAuthenticated) {
            // Проверяем изменение лицензии
            char status[64] = {0};
            BOOL isValid = FALSE;
            DWORD validUntil = 0;
            
            if (CallGetLicenseStatusInternal(status, sizeof(status), &isValid, &validUntil)) {
                // Уведомляем главное окно об изменении
                PostMessage(hWnd, WM_USER + 100, isValid ? 1 : 0, validUntil);
            }
        }
    }
    return 0;
}

// Запуск потока опроса лицензии
HANDLE StartLicensePolling(HWND hWnd) {
    return CreateThread(NULL, 0, LicensePollingThread, hWnd, 0, NULL);
}

// Обработка сообщения об изменении лицензии
void OnLicenseChanged(HWND hWnd, WPARAM wParam, LPARAM lParam) {
    BOOL hasLicense = (BOOL)wParam;
    DWORD validUntil = (DWORD)lParam;
    
    UserInfo userInfo = GetCurrentUserInfo();
    userInfo.hasLicense = hasLicense;
    userInfo.licenseValidUntil = validUntil;
    
    UpdateLicenseStatus(hWnd, userInfo);
    
    if (hasLicense) {
        MessageBox(hWnd, L"License activated successfully!", L"AVAA", MB_OK);
    }
}