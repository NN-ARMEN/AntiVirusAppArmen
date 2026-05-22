#pragma once
#include <windows.h>
#include <winhttp.h>
#include <string>
#include <vector>
#include <map>

#pragma comment(lib, "winhttp.lib")

// Структура HTTP ответа
struct HttpResponse {
    int statusCode;
    std::string body;
    std::wstring errorMessage;
    
    HttpResponse() : statusCode(0) {}
};

// Класс для HTTP запросов через WinHTTP
class WebClient {
public:
    WebClient();
    ~WebClient();
    
    // Инициализация
    BOOL Initialize();
    void Cleanup();
    
    // Настройка эндпоинтов
    void SetAuthEndpoint(const std::wstring& url);
    void SetLicenseEndpoint(const std::wstring& url);
    void SetActivateEndpoint(const std::wstring& url);
    
    // HTTP методы
    HttpResponse Post(const std::wstring& url, const std::string& body, const std::string& authToken = "");
    HttpResponse Get(const std::wstring& url, const std::string& authToken = "");
    
private:
    HINTERNET m_hSession;
    
    std::wstring m_authUrl;
    std::wstring m_licenseUrl;
    std::wstring m_activateUrl;
    
    std::string SendRequest(const std::wstring& method, const std::wstring& url, 
                            const std::string& body, const std::string& authToken);
    bool ParseUrl(const std::wstring& url, std::wstring& host, int& port, std::wstring& path, bool& isHttps);
};