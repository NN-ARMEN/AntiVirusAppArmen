#pragma once
#include <windows.h>
#include <winhttp.h>
#include <string>
#include <map>

#pragma comment(lib, "winhttp.lib")

struct HttpResponse {
    int statusCode;
    std::string body;
    std::wstring errorMessage;
    HttpResponse() : statusCode(0) {}
};

class WebClient {
public:
    WebClient();
    ~WebClient();
    BOOL Initialize();
    void Cleanup();
    void SetAuthEndpoint(const std::wstring& url);
    void SetLicenseEndpoint(const std::wstring& url);
    void SetActivateEndpoint(const std::wstring& url);
    HttpResponse Post(const std::wstring& url, const std::string& body, const std::string& authToken = "");
    HttpResponse Get(const std::wstring& url, const std::string& authToken = "");
private:
    HINTERNET m_hSession;
    std::wstring m_authUrl, m_licenseUrl, m_activateUrl;
    std::string SendRequest(const std::wstring& method, const std::wstring& url, const std::string& body, const std::string& authToken);
    bool ParseUrl(const std::wstring& url, std::wstring& host, int& port, std::wstring& path, bool& isHttps);
};