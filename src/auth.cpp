#include "auth.h"
#include <chrono>
#include <thread>

// Глобальные данные (хранятся только в оперативной памяти)
static AuthTokens g_authData = {};
static LicenseData g_licenseData = {};
static std::mutex g_authMutex;
static std::mutex g_licenseMutex;
static WebClient g_webClient;
static HANDLE g_tokenRefreshThread = NULL;
static HANDLE g_licenseRefreshThread = NULL;
static volatile BOOL g_stopThreads = FALSE;
static volatile BOOL g_initialized = FALSE;

// Вспомогательные функции
static DWORD GetCurrentTimestamp() {
    return (DWORD)time(NULL);
}

static void ParseTokensFromResponse(const std::string& response, AuthTokens& tokens) {
    tokens.accessToken = JsonParser::GetStringValue(response, "access_token");
    tokens.refreshToken = JsonParser::GetStringValue(response, "refresh_token");
    tokens.accessTokenExpiry = GetCurrentTimestamp() + JsonParser::GetIntValue(response, "expires_in");
    tokens.refreshTokenExpiry = GetCurrentTimestamp() + JsonParser::GetIntValue(response, "refresh_expires_in");
}

static void ParseLicenseFromResponse(const std::string& response, LicenseData& license) {
    license.licenseTicket = response;
    license.status = JsonParser::GetStringValue(response, "status");
    license.validUntil = JsonParser::GetIntValue(response, "valid_until");
    license.gracePeriod = JsonParser::GetIntValue(response, "grace_period");
    license.productName = JsonParser::GetStringValue(response, "product_name");
    license.isValid = (license.status == "active" && license.validUntil > GetCurrentTimestamp());
}

// Поток для периодического обновления токенов
DWORD WINAPI TokenRefreshThreadProc(LPVOID lpParam) {
    while (!g_stopThreads) {
        Sleep(60000); // Проверяем каждую минуту
        
        std::lock_guard<std::mutex> lock(g_authMutex);
        
        if (!g_authData.isAuthenticated) continue;
        
        DWORD now = GetCurrentTimestamp();
        
        // Если access token истекает через менее чем 5 минут, обновляем
        if (g_authData.accessTokenExpiry > now && 
            (g_authData.accessTokenExpiry - now) < 300) {
            
            std::map<std::string, std::string> request;
            request["refresh_token"] = g_authData.refreshToken;
            std::string body = JsonParser::SerializeObject(request);
            
            HttpResponse response = g_webClient.Post(L"https://your-api.com/auth/refresh", body);
            
            if (response.statusCode == 200) {
                ParseTokensFromResponse(response.body, g_authData);
            }
        }
    }
    return 0;
}

// Поток для периодического обновления лицензии
DWORD WINAPI LicenseRefreshThreadProc(LPVOID lpParam) {
    while (!g_stopThreads) {
        Sleep(3600000); // Проверяем каждый час
        
        std::lock_guard<std::mutex> authLock(g_authMutex);
        std::lock_guard<std::mutex> licenseLock(g_licenseMutex);
        
        if (!g_authData.isAuthenticated || !g_licenseData.isValid) continue;
        
        DWORD now = GetCurrentTimestamp();
        
        // Если лицензия истекает через менее чем 24 часа, обновляем
        if (g_licenseData.validUntil > now && 
            (g_licenseData.validUntil - now) < 86400) {
            
            HttpResponse response = g_webClient.Get(L"https://your-api.com/license/status", 
                                                     g_authData.accessToken);
            
            if (response.statusCode == 200) {
                ParseLicenseFromResponse(response.body, g_licenseData);
            } else if (response.statusCode == 401) {
                // Токен истёк, пробуем обновить
                // Упрощённо - помечаем как не аутентифицированных
                g_authData.isAuthenticated = false;
            }
        }
    }
    return 0;
}

// Инициализация
BOOL InitAuthSystem() {
    if (g_initialized) return TRUE;
    
    if (!g_webClient.Initialize()) {
        return FALSE;
    }
    
    // Настройка эндпоинтов (замените на реальные URL)
    g_webClient.SetAuthEndpoint(L"https://your-api.com/auth/login");
    g_webClient.SetLicenseEndpoint(L"https://your-api.com/license/status");
    g_webClient.SetActivateEndpoint(L"https://your-api.com/license/activate");
    
    memset(&g_authData, 0, sizeof(g_authData));
    memset(&g_licenseData, 0, sizeof(g_licenseData));
    
    g_stopThreads = FALSE;
    g_initialized = TRUE;
    
    return TRUE;
}

void CleanupAuthSystem() {
    if (!g_initialized) return;
    
    StopBackgroundThreads();
    g_webClient.Cleanup();
    g_initialized = FALSE;
}

void StartBackgroundThreads() {
    if (g_tokenRefreshThread) return;
    
    g_stopThreads = FALSE;
    g_tokenRefreshThread = CreateThread(NULL, 0, TokenRefreshThreadProc, NULL, 0, NULL);
    g_licenseRefreshThread = CreateThread(NULL, 0, LicenseRefreshThreadProc, NULL, 0, NULL);
}

void StopBackgroundThreads() {
    g_stopThreads = TRUE;
    
    if (g_tokenRefreshThread) {
        WaitForSingleObject(g_tokenRefreshThread, 5000);
        CloseHandle(g_tokenRefreshThread);
        g_tokenRefreshThread = NULL;
    }
    
    if (g_licenseRefreshThread) {
        WaitForSingleObject(g_licenseRefreshThread, 5000);
        CloseHandle(g_licenseRefreshThread);
        g_licenseRefreshThread = NULL;
    }
}

// RPC методы для аутентификации
void AuthLogin(const char* username, const char* password, BOOL* success, char* errorMessage, int errorSize) {
    *success = FALSE;
    
    if (!username || !password) {
        if (errorMessage && errorSize > 0) {
            strncpy_s(errorMessage, errorSize, "Invalid credentials", errorSize - 1);
        }
        return;
    }
    
    std::map<std::string, std::string> request;
    request["username"] = username;
    request["password"] = password;
    std::string body = JsonParser::SerializeObject(request);
    
    HttpResponse response = g_webClient.Post(L"https://your-api.com/auth/login", body);
    
    if (response.statusCode == 200) {
        std::lock_guard<std::mutex> lock(g_authMutex);
        ParseTokensFromResponse(response.body, g_authData);
        g_authData.username = username;
        g_authData.isAuthenticated = true;
        
        *success = TRUE;
        StartBackgroundThreads();
    } else {
        if (errorMessage && errorSize > 0) {
            strncpy_s(errorMessage, errorSize, "Authentication failed", errorSize - 1);
        }
    }
}

void AuthLogout(BOOL* success) {
    *success = FALSE;
    
    std::lock_guard<std::mutex> authLock(g_authMutex);
    std::lock_guard<std::mutex> licenseLock(g_licenseMutex);
    
    if (g_authData.isAuthenticated) {
        // Отправляем запрос на выход
        g_webClient.Post(L"https://your-api.com/auth/logout", "", g_authData.accessToken);
        
        // Очищаем данные
        g_authData = {};
        g_licenseData = {};
        
        *success = TRUE;
    }
}

void GetCurrentUser(char* username, int usernameSize, BOOL* isAuthenticated) {
    std::lock_guard<std::mutex> lock(g_authMutex);
    
    *isAuthenticated = g_authData.isAuthenticated;
    if (username && g_authData.isAuthenticated) {
        strncpy_s(username, usernameSize, g_authData.username.c_str(), usernameSize - 1);
    } else if (username && usernameSize > 0) {
        username[0] = 0;
    }
}

void GetLicenseStatus(char* status, int statusSize, BOOL* isValid, DWORD* validUntil) {
    std::lock_guard<std::mutex> lock(g_licenseMutex);
    
    *isValid = g_licenseData.isValid;
    *validUntil = g_licenseData.validUntil;
    
    if (status && statusSize > 0) {
        strncpy_s(status, statusSize, g_licenseData.status.c_str(), statusSize - 1);
    }
}

void ActivateProduct(const char* activationCode, BOOL* success, char* errorMessage, int errorSize) {
    *success = FALSE;
    
    if (!activationCode) {
        if (errorMessage && errorSize > 0) {
            strncpy_s(errorMessage, errorSize, "Invalid activation code", errorSize - 1);
        }
        return;
    }
    
    std::lock_guard<std::mutex> authLock(g_authMutex);
    
    if (!g_authData.isAuthenticated) {
        if (errorMessage && errorSize > 0) {
            strncpy_s(errorMessage, errorSize, "Not authenticated", errorSize - 1);
        }
        return;
    }
    
    std::map<std::string, std::string> request;
    request["activation_code"] = activationCode;
    std::string body = JsonParser::SerializeObject(request);
    
    HttpResponse response = g_webClient.Post(L"https://your-api.com/license/activate", body, g_authData.accessToken);
    
    if (response.statusCode == 200) {
        std::lock_guard<std::mutex> licenseLock(g_licenseMutex);
        ParseLicenseFromResponse(response.body, g_licenseData);
        *success = TRUE;
    } else {
        if (errorMessage && errorSize > 0) {
            strncpy_s(errorMessage, errorSize, "Activation failed", errorSize - 1);
        }
    }
}

BOOL HasValidLicense() {
    std::lock_guard<std::mutex> lock(g_licenseMutex);
    return g_licenseData.isValid;
}

void ConfigureEndpoints(const std::wstring& authUrl, 
                        const std::wstring& licenseUrl, 
                        const std::wstring& activateUrl) {
    g_webClient.SetAuthEndpoint(authUrl);
    g_webClient.SetLicenseEndpoint(licenseUrl);
    g_webClient.SetActivateEndpoint(activateUrl);
}