#include "web_client.h"
#include <sstream>

WebClient::WebClient() : m_hSession(NULL) {
}

WebClient::~WebClient() {
    Cleanup();
}

BOOL WebClient::Initialize() {
    m_hSession = WinHttpOpen(L"AVAA-Client/1.0", 
                             WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                             WINHTTP_NO_PROXY_NAME, 
                             WINHTTP_NO_PROXY_BYPASS, 0);
    return (m_hSession != NULL);
}

void WebClient::Cleanup() {
    if (m_hSession) {
        WinHttpCloseHandle(m_hSession);
        m_hSession = NULL;
    }
}

void WebClient::SetAuthEndpoint(const std::wstring& url) {
    m_authUrl = url;
}

void WebClient::SetLicenseEndpoint(const std::wstring& url) {
    m_licenseUrl = url;
}

void WebClient::SetActivateEndpoint(const std::wstring& url) {
    m_activateUrl = url;
}

bool WebClient::ParseUrl(const std::wstring& url, std::wstring& host, int& port, std::wstring& path, bool& isHttps) {
    URL_COMPONENTS urlComp = { sizeof(urlComp) };
    wchar_t hostName[256] = {0};
    wchar_t urlPath[1024] = {0};
    
    urlComp.lpszHostName = hostName;
    urlComp.dwHostNameLength = sizeof(hostName) / sizeof(wchar_t);
    urlComp.lpszUrlPath = urlPath;
    urlComp.dwUrlPathLength = sizeof(urlPath) / sizeof(wchar_t);
    
    if (!WinHttpCrackUrl(url.c_str(), 0, 0, &urlComp)) {
        return false;
    }
    
    host = hostName;
    port = urlComp.nPort;
    path = urlPath;
    isHttps = (urlComp.nScheme == INTERNET_SCHEME_HTTPS);
    
    return true;
}

std::string WebClient::SendRequest(const std::wstring& method, const std::wstring& url, 
                                    const std::string& body, const std::string& authToken) {
    std::wstring host;
    int port;
    std::wstring path;
    bool isHttps;
    
    if (!ParseUrl(url, host, port, path, isHttps)) {
        return "";
    }
    
    HINTERNET hConnect = WinHttpConnect(m_hSession, host.c_str(), port, 0);
    if (!hConnect) {
        return "";
    }
    
    DWORD flags = (isHttps) ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, method.c_str(), path.c_str(), 
                                             NULL, NULL, NULL, flags);
    
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        return "";
    }
    
    // Добавляем заголовки
    std::wstring headers = L"Content-Type: application/json\r\n";
    if (!authToken.empty()) {
        std::wstring authHeader = L"Authorization: Bearer " + 
                                   std::wstring(authToken.begin(), authToken.end()) + L"\r\n";
        headers += authHeader;
    }
    
    // Отправляем запрос
    BOOL result = WinHttpSendRequest(hRequest, headers.c_str(), headers.length(),
                                      (LPVOID)body.c_str(), body.length(), body.length(), 0);
    
    if (result) {
        result = WinHttpReceiveResponse(hRequest, NULL);
    }
    
    // Читаем ответ
    std::string response;
    if (result) {
        DWORD bytesAvailable = 0;
        do {
            WinHttpQueryDataAvailable(hRequest, &bytesAvailable);
            if (bytesAvailable > 0) {
                char* buffer = new char[bytesAvailable + 1];
                DWORD bytesRead = 0;
                WinHttpReadData(hRequest, buffer, bytesAvailable, &bytesRead);
                buffer[bytesRead] = 0;
                response += buffer;
                delete[] buffer;
            }
        } while (bytesAvailable > 0);
    }
    
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    
    return response;
}

HttpResponse WebClient::Post(const std::wstring& url, const std::string& body, const std::string& authToken) {
    HttpResponse result;
    result.body = SendRequest(L"POST", url, body, authToken);
    result.statusCode = 200; // Упрощённо
    return result;
}

HttpResponse WebClient::Get(const std::wstring& url, const std::string& authToken) {
    HttpResponse result;
    result.body = SendRequest(L"GET", url, "", authToken);
    result.statusCode = 200;
    return result;
}