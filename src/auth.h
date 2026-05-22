#pragma once
#include <windows.h>
#include <string>
#include <mutex>
#include "web_client.h"
#include "json_parser.h"

// Структуры данных для аутентификации
struct AuthTokens {
    std::string accessToken;
    std::string refreshToken;
    DWORD accessTokenExpiry;   // Unix timestamp
    DWORD refreshTokenExpiry;
    std::string username;
    bool isAuthenticated;
};

// Структуры данных для лицензии
struct LicenseData {
    std::string licenseTicket;
    std::string status;        // "active", "expired", "none"
    DWORD validUntil;          // Unix timestamp
    DWORD gracePeriod;
    bool isValid;
    std::string productName;
};

// Инициализация и очистка
BOOL InitAuthSystem();
void CleanupAuthSystem();

// RPC методы для аутентификации (вызываются из rpc_server)
void AuthLogin(const char* username, const char* password, BOOL* success, char* errorMessage, int errorSize);
void AuthLogout(BOOL* success);
void GetCurrentUser(char* username, int usernameSize, BOOL* isAuthenticated);
void GetLicenseStatus(char* status, int statusSize, BOOL* isValid, DWORD* validUntil);
void ActivateProduct(const char* activationCode, BOOL* success, char* errorMessage, int errorSize);
BOOL HasValidLicense();

// Внутренние методы для управления фоновыми потоками
void StartBackgroundThreads();
void StopBackgroundThreads();

// Конфигурация эндпоинтов (для тестирования)
void ConfigureEndpoints(const std::wstring& authUrl, 
                        const std::wstring& licenseUrl, 
                        const std::wstring& activateUrl);

// Вспомогательные функции для парсинга
void ParseTokensFromResponse(const std::string& response, AuthTokens& tokens);
void ParseLicenseFromResponse(const std::string& response, LicenseData& license);